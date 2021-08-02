.code16
.text
.globl start

#include "x86-consts.h"
#include "bootsector.h"

start:
	/*
	 * We can't handle being interrupted and the registers might be in an
	 * invalid state at the outside, so clear them all.
	 */
	cli
	xorw %ax, %ax
	movw %ax, %ds // Data  segment.
	movw %ax, %es // Extra segment.
	movw %ax, %ss // Stack segment.

	call enable_a20

	call check_long_mode_support
	cmpw $1, %ax
	jne start.no_long_mode

	// Move to 32-bit protected mode.
	lgdtl gdtdescr
	movl $X86_CR0_PROTECTED_MODE, %eax
	movl %eax, %cr0
	ljmpl $GDT_SEGMENT_CODE32, $start32

start.no_long_mode:
	movw $panic_str, %si
	call bios_say
	hlt

// Outputs string pointed to by SI.
bios_say:
	// 'teletype output' - see https://en.wikipedia.org/wiki/INT_10H
	movb $0xe, %ah
	xorw %bx, %bx
bios_say.loop:
	lodsb // Load byte at DS:SI into AL.
	cmpb $0, %al
	je bios_say.done // Exit on null terminator.
	int $0x10
	jmp bios_say.loop
bios_say.done:
	ret

/*
 * Once upon a time 1 MiB was considered a huge amount of memory. Thus the
 * weirdness that is segmented memory meant that 16-bit x86 could specify an
 * address exceeding this 'huge' capacity, e.g. 0xffff:0xffff = 0x10ffef =
 * 0x100000 (1 MiB) + 0xffef (just under 64 KiB).
 *
 * Intel thought it'd be a pretty great idea to round things out at 1 MiB by
 * 'wrapping' memory around so anything exceeding 0xffff:0x0010 would
 * 'wrap around' to 0x0000:0x0000.
 *
 * Inevitably some developers decided that it was a great idea to rely on this
 * behaviour, which left Intel with the Old New Thing problem of maintaining
 * backwards compatibility in newer processors.
 *
 * The ability to enable/disable A20 is the solution - it refers to 'address
 * line 20' (in base 0, so 21st bit), e.g. if enabled then no wrap-around,
 * otherwise wrap-around.
 *
 * For compatibility (some) x86 processors start real mode with A20 disabled.
 * For obvious reasons we have to enable it.
 *
 * For more on this horror read https://www.win.tue.nl/~aeb/linux/kbd/A20.html
 */
enable_a20:
	// Is A20 already enabled? Then nothing to do.
	call check_a20
	cmpw $1, %ax
	je enable_a20.done

	// Attempt 1: BIOS enable.
	movw $0x2401, %ax
	int $0x15
	call check_a20
	cmpw $1, %ax
	je enable_a20.done

	// Attempt 2: Fast A20.
	inb $0x92, %al
	orb $0b10, %al
	outb %al, $0x92
	call check_a20
	cmpw $1, %ax
	je enable_a20.done

	// Nothing worked, we cannot continue.
	movw $panic_str, %si
	call bios_say
	hlt

enable_a20.done:
	ret

/*
 * Checks whether A20 is enabled. It achieves this by firstly checking if the
 * bootsector magic number, 0xaa55 (located at 0x0000:0x7dfe, e.g. bootsector
 * load address 0x7c00 + 512 - 2), differs from the A20-disabled wrap around at
 * 0xffff:0x7e0e. If they differ then A20 cannot be enabled. If by chance they
 * happen to be the same, we set 0x0000:0x7dfe to an arbitrary value and check
 * again. If they mirror one another then A20 is definitely disabled. The magic
 * number is restored afterwards.
 *
 * Sets AX to 1 if enabled, 0 if disabled.
 */
check_a20:
	// Clear DS.
	xorw %ax, %ax
	movw %ax, %ds
	// Set DI to 0x7dfe so %ds:%di == 0x0000: 0x7dfe.
	movw $0x7dfe, %di
	// Store 0x0000:0x7dfe in %ax and copy to %bx.
	movw %ds:0(%di), %ax
	movw %ax, %bx
	// Set ES to 0xffff.
	movw $0xffff, %ax
	movw %ax, %es
	// Set SI to 0x7e0e so %es:%si == 0xffff:0x7e0e.
	movw $0x7e0e, %si
	// If 0x0000:0x7dfe != 0xffff:0x7e0e then A20 must be enabled.
	cmpw %es:0(%si), %ax
	jne check_a20.enabled
	/*
	 * Just in case they happen to match by chance, set 0x0000:0x7dfe to an
	 * arbitrary value.
	 */
	movw $0xbeef, %ax
	movw %ax, %ds:0(%di)
	// Check if they match, if they do now then A20 is definitely disabled.
	cmpw %es:0(%si), %ax
	// Before returning, restore bootsector magic value.
	movw %bx, %ds:0(%di)
	je check_a20.disabled
check_a20.enabled:
	movw $1, %ax
	ret
check_a20.disabled:
	movw $0, %ax
	ret

/*
 * Determines whether CPU supports long mode (e.g. 64-bit).
 * Sets AX 1 if so, 0 if not.
 */
check_long_mode_support:
	xorl %edx, %edx
	movl $0x80000001, %eax
	cpuid
	andl $X86_LONGMODE_FLAG, %edx
	jz check_long_mode_support.unsupported
	movw $1, %ax
	ret
check_long_mode_support.unsupported:
	movw $0, %ax
	ret

.code32

start32:
	movw $GDT_SEGMENT_DATA, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	xorl %eax, %eax
	movw %ax, %fs
	movw %ax, %gs

	// Set up page tables.

	// Clear 4 pages at 0x1000, 0x2000, 0x3000, 0x4000.
	movl $0x1000, %edi
	movl $0x1000, %ecx
	rep stosl
	// Point PGT[0] -> PUD[0] -> PMD[0] -> PTE[0]
	movl $0x2003, 0x1000
	movl $0x3003, 0x2000
	movl $0x4003, 0x3000

	// Set up linear mapping for first 2 MiB.
	movl $3, %eax
	movl $0x4000, %edi
	movl $512, %ecx
start32.setpte_loop:
	movl %eax, (%edi)
	addl $0x1000, %eax
	addl $8, %edi
	loop start32.setpte_loop

	// Assign PGT to CR3.
	movl $0x1000, %edi
	movl %edi, %cr3

	// Enable PAE.
	movl %cr4, %eax
	orl $X86_CR4_PAE, %eax
	movl %eax, %cr4

	// Enable long mode.
	movl $X86_MFR_EFER, %ecx
	rdmsr
	orl $X86_MFR_EFER_LME, %eax
	wrmsr

	// Enable paging.
	movl %cr0, %eax
	orl $X86_CR0_PAGED_MODE, %eax
	movl %eax, %cr0

	// Jump into long mode.
	ljmpl $GDT_SEGMENT_CODE64, $start64

.code64
start64:
	movw $GDT_SEGMENT_DATA, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	xorq %rax, %rax
	movw %ax, %fs
	movw %ax, %gs

	movq $start, %rsp
	call load
	hlt

panic_str:
	.asciz "panic"

gdt:
	.quad MAKE_GDTE(0, 0) // Null, can't select GDTE 0.
	.quad MAKE_GDTE(X86_GDTE_FLAG_4K_GRANULARITY | X86_GDTE_FLAG_32BIT_PROTECTED,
			X86_GDTE_ACCESS_PRESENT | X86_GDTE_ACCESS_NONSYS |
			X86_GDTE_ACCESS_EXEC | X86_GDTE_ACCESS_RW) // GDT_SEGMENT_CODE32_INDEX
	.quad MAKE_GDTE(X86_GDTE_FLAG_4K_GRANULARITY | X86_GDTE_FLAG_64BIT_CODE,
			X86_GDTE_ACCESS_PRESENT | X86_GDTE_ACCESS_NONSYS |
			X86_GDTE_ACCESS_EXEC | X86_GDTE_ACCESS_RW) // GDT_SEGMENT_CODE64_INDEX
	.quad MAKE_GDTE(X86_GDTE_FLAG_4K_GRANULARITY | X86_GDTE_FLAG_32BIT_PROTECTED,
			X86_GDTE_ACCESS_PRESENT | X86_GDTE_ACCESS_NONSYS |
			X86_GDTE_ACCESS_RW)                        // GDT_SEGMENT_DATA_INDEX
gdtdescr:
	.short gdtdescr - gdt - 1 // size
	.long  gdt // offset
