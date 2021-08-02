.code16
.text
.globl start

#include "x86-consts.h"
#include "bootsector.h"

// We are struggling to fit code into boot sector so skip checks and assume A20
// enabled (it is in qemu) and 64-bit capable CPU.
#define SKIP_CHECKS

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

#ifndef SKIP_CHECKS
	call enable_a20

	call check_long_mode_support
	cmpw $1, %ax
	jne start.no_long_mode
#endif

	// Move to 32-bit protected mode.
	lgdtl gdtdescr
	movl $X86_CR0_PROTECTED_MODE, %eax
	movl %eax, %cr0
	ljmpl $GDT_SEGMENT_CODE32, $start32

#ifndef SKIP_CHECKS
start.no_long_mode:
	movw $panic_str, %si
	call bios_say
	hlt
#endif

#ifndef SKIP_CHECKS
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
#endif

#ifndef SKIP_CHECKS
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
#endif

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
	//
	// Perform 1 GiB 'gigantic' page mappings (only PGT, PUD with PSE bit
	// set in PUD) from PA 0 to 1 GiB at VAs:
	//   * 0
	//   * X86_KERNEL_DIRECT_MAP_BASE
	//   * X86_KERNEL_ELF_BASE

	// Clear memory in range [0x1000, 0x5000).
	// (eax was cleared above).
	// This memory is safe to use as is located in 'conventional memory'.
	movl $0x1000, %edi
	movl $0x4000, %ecx
	rep stosb

	// (We can use 32-bit moves below because the page table bits we are
	//  manipulating are all within lower 32).

	// PGE for VA 0 -> PUD at 0x2000.
	movl $(X86_PAGE_FLAG_KERNEL | 0x2000), 0x1000
	// PGE for VA X86_KERNEL_DIRECT_MAP_BASE -> PUD at 0x3000.
	movl $(X86_PAGE_FLAG_KERNEL | 0x3000), (0x1000 + 8 * X86_KERNEL_DIRECT_MAP_BASE_PGD_OFFSET)
	// PGE for VA X86_KERNEL_ELF_BASE -> PUD at 0x4000.
	movl $(X86_PAGE_FLAG_KERNEL | 0x4000), (0x1000 + 8 * X86_KERNEL_ELF_BASE_PGD_OFFSET)

	// Add 'gigantic' PUDs pointing at PA 0.
	movl $(X86_PAGE_FLAG_KERNEL | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), 0x2000
	movl $(X86_PAGE_FLAG_KERNEL | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), 0x3000
	movl $(X86_PAGE_FLAG_KERNEL | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), 0x4000

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

#ifdef SKIP_CHECKS
panic_str:
	.asciz "panic"
#endif

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
