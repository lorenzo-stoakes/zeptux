#include "x86-consts.h"
#include "bootsector.h"

.text

.code16

boot2:
	call enter_video_mode_3

	call get_e820

	// Move to 32-bit protected mode.
	lgdtl gdtdescr
	movl $X86_CR0_PROTECTED_MODE, %eax
	movl %eax, %cr0
	ljmpl $GDT_SEGMENT_CODE32, $start32

enter_video_mode_3:
	mov $3, %ax 	// Set video mode to 3 (80x25)
	int $0x10
	ret

// Retrieve e820 memory layout information and store it in the early_boot_info
// struct located at EARLY_BOOT_INFO_ADDRESS_PHYS.
get_e820:
	xorl %ebx, %ebx
	movl $X86_E820_MAGIC_NUMBER, %edx // Magic number == "SMAP".
	movl $20, %ecx 	// We don't care about the ACPI extended bits.

	// We store the target address in ES:DI. Offset by 10 bytes to locate
	// the first entry in the array.
	xorw %ax, %ax
	movw %ax, %es
	movw $X86_EARLY_BOOT_INFO_ADDRESS_PHYS, %di
	addw $10, %di

	movl $0xe820, %eax
	int $0x15
	jc get_e820.unsupported

	cmpl %eax, %edx
	jne get_e820.call_failed

	xorw %bp, %bp   // Use for counter.

	// We got an entry just to check it worked, now reset and do it again,
	// this time for real.
	xorl %ebx, %ebx
get_e820.loop:
	// These get clobbered on every call.
	movl $0xe820, %eax
	movl $20, %ecx
	movl $X86_E820_MAGIC_NUMBER, %edx
	int $0x15

	jc get_e820.done   // Now carry set means we were already done.
	jcxz get_e820.skip // Skip entries with zero entry size.
	cmpb $20, %cl 	   // If CL is anything other than 20 bytes then something's borked.
	jne get_e820.unexpected_length
	movl %es:8(%di), %ecx
	orl %es:12(%di), %ecx
	jz get_e820.skip // Skip entries with entries OF zero size.

	// OK, we're good, so read more.
	addw $20, %di
	incw %bp
get_e820.skip:
	// If EBX is 0 then there's nothing left to process.
	testl %ebx, %ebx
	jnz get_e820.loop
get_e820.done:
	movw %bp, X86_EARLY_BOOT_INFO_ADDRESS_PHYS + 8 // Store entry count.
	// Restore clobbered regs and clear ES.
	xorl %eax, %eax
	xorl %ebx, %ebx
	xorl %ecx, %ecx
	movw %ax, %es
	// Carry flag could be set by e820 call so clear it.
	clc
	ret

get_e820.unsupported:
	movw $e820_unsupported_str, %si
	call bios_say
	hlt

get_e820.call_failed:
	movw $e820_call_failed_str, %si
	call bios_say
	hlt

get_e820.unexpected_length:
	movw $e820_unexpected_length_str, %si
	call bios_say
	hlt

#include "bios_say.S"

e820_unsupported_str:
	.asciz "panic: e820 call unsupported!"

e820_call_failed_str:
	.asciz "panic: e820 call failed!"

e820_unexpected_length_str:
	.asciz "panic: e820 call unexpected entry length!"

.code32

start32:
	movw $GDT_SEGMENT_DATA, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	xorw %ax, %ax
	movw %ax, %fs
	movw %ax, %gs

	// Set up page tables.
	//
	// Perform 1 GiB 'gigantic' page mappings (only PGT, PUD with PSE bit
	// set in PUD) from PA 0 to 1 GiB at VAs:
	//   * 0
	//   * X86_KERNEL_DIRECT_MAP_BASE
	//   * X86_KERNEL_ELF_BASE

	// Clear 4 pages of memory to be used as page tables.
	xorl %eax, %eax
	movl $X86_EARLY_PGD, %edi
	movl $0x4000, %ecx
	rep stosb

	// (We can use 32-bit moves below because the page table bits we are
	//  manipulating are all within lower 32).

	// PGE for VA 0 -> PUD at 0x2000.
	movl $(X86_PAGE_FLAG_KERNEL | X86_EARLY_PUD_DIRECT0), X86_EARLY_PGD
	// PGE for VA X86_KERNEL_DIRECT_MAP_BASE -> PUD at 0x3000.
	movl $(X86_PAGE_FLAG_KERNEL | X86_EARLY_PUD_DIRECT_MAP), (X86_EARLY_PGD + 8 * X86_KERNEL_DIRECT_MAP_BASE_PGD_OFFSET)
	// PGE for VA X86_KERNEL_ELF_BASE -> PUD at 0x4000.
	movl $(X86_PAGE_FLAG_KERNEL | X86_EARLY_PUD_KERNEL_ELF), (X86_EARLY_PGD + 8 * X86_KERNEL_ELF_BASE_PGD_OFFSET)

	// Add 'gigantic' PUDs pointing at PA 0.
	movl $(X86_PAGE_FLAG_KERNEL | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), X86_EARLY_PUD_DIRECT0
	movl $(X86_PAGE_FLAG_KERNEL | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), X86_EARLY_PUD_DIRECT_MAP
	movl $(X86_PAGE_FLAG_KERNEL | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), X86_EARLY_PUD_KERNEL_ELF

	// Assign PGT to CR3.
	movl $X86_EARLY_PGD, %edi
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

	xorw %ax, %ax
	movw %ax, %fs
	movw %ax, %gs

	// Setup stack.
	movq $X86_KERNEL_INIT_STACK, %rsp
	// Call the kernel ELF loader.
	call load

	// If we reach here, something has gone wrong!
	hlt
start64.loop:
	jmp start64.loop

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
