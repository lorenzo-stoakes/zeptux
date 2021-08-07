#include "x86-consts.h"
#include "bootsector.h"

.text

.code16

boot2:
	// Move to 32-bit protected mode.
	lgdtl gdtdescr
	movl $X86_CR0_PROTECTED_MODE, %eax
	movl %eax, %cr0
	ljmpl $GDT_SEGMENT_CODE32, $start32

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

.data

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
