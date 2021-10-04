#include "x86-consts.h"
#include "bootsector.h"
#include "config.h"

.text

.code16

boot2:
#ifdef CONFIG_BOOTLOADER_BIOS
	call bios_load_kernel_elf
#endif
	call set_video_mode
	call get_e820
	call disable_pic

	// Move to 32-bit protected mode.
	lgdtl gdtdescr
	movl $X86_CR0_PROTECTED_MODE, %eax
	movl %eax, %cr0
	ljmpl $GDT_SEGMENT_CODE32, $start32

set_video_mode:
	movw $3, %ax 	// Set video mode to 3 (80x25)
	int $0x10
	ret

// Retrieve e820 memory layout information and store it in the early_boot_info
// struct located at EARLY_BOOT_INFO_ADDRESS_PHYS.
get_e820:
	xorl %ebx, %ebx
	movl $X86_E820_MAGIC_NUMBER, %edx // Magic number == "SMAP".
	movl $20, %ecx 	// We don't care about the ACPI extended bits.

	// We store the target address in ES:DI.
	xorw %ax, %ax
	movw %ax, %es
	movw $X86_EARLY_BOOT_INFO_E820_ENTRY_ARRAY_ADDRESS_PHYS, %di

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
	movw %bp, X86_EARLY_BOOT_INFO_E820_ENTRY_COUNT_ADDRESS_PHYS // Store entry count.
	// Clear upper bytes of 32-bit value.
	movw $0, X86_EARLY_BOOT_INFO_E820_ENTRY_COUNT_ADDRESS_PHYS + 2

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
	jmp get_e820.forever

get_e820.call_failed:
	movw $e820_call_failed_str, %si
	call bios_say
	jmp get_e820.forever

get_e820.unexpected_length:
	movw $e820_unexpected_length_str, %si
	call bios_say
	jmp get_e820.forever

get_e820.forever:
	jmp get_e820.forever

disable_pic:
	mov $0xff, %al
	outb %al, $X86_PIC1_DATA_PORT
	outb %al, $X86_PIC2_DATA_PORT
	ret

#ifdef CONFIG_BOOTLOADER_BIOS
bios_load_kernel_elf:
	// We store the remaining number of sectors to load in %cx and the
	// current number to load in %bx.

	movw $BOOT_DRIVE_NUM, %dx
	movw $BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS, %si

	// This will be calculated and set in the image on build.
	movl KERNEL_IMAGE_SIZE_MEM_ADDRESS, %ecx
	shrl $9, %ecx // Convert to sectors.
	incw %cx      // Effectively round up by always loading 1 sector more.

	// Initialise DAP.
	movb $0x10, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS)  // Size of DAP
	movb $0, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 1) // unused
	// (next 2 bytes - number of sectors to load.)
	// Where to load offset:
	movw $BIOS_KERNEL_ELF_LOAD_PHYS_ADDRESS, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 4)
	// Where to load segment:
	movw $0, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 6)
	// Sector offset (loading after boot sector and stage 2 sectors):
	movl $STAGE2_SECTORS + 1, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 8)
	movl $0, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 12)

	// Determine if we only need to load sectors less than those in the
	// current segment...
	movw %cx, %bx
	cmpw $BIOS_KERNEL_ELF_LOAD_SEGMENT0_SECTORS_REMAINING, %bx
	jle bios_load_kernel_elf.loop
	// ...If not we can only load until the end of the segment to avoid
	// weird behaviour.
	mov $BIOS_KERNEL_ELF_LOAD_SEGMENT0_SECTORS_REMAINING, %bx

bios_load_kernel_elf.loop:
	// Set the number of sectors to load.
	movw %bx, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 2)

	// Execute 'extended read sectors from drive' command.
	movw $0x4200, %ax
	int $0x13
	jc bios_load_kernel_elf.fault

	// Subtract what we have loaded from remaining.
	subw %bx, %cx

	// Are we done yet?
	cmpw $0, %cx
	je bios_load_kernel_elf.done

	// Zero target address.
	movw $0, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 4)
	// Increment segment.
	movw (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 6), %ax
	addw $0x1000, %ax
	movw %ax, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 6)

	// Offset by number of sectors loaded.
	movl (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 8), %eax
	addw %bx, %ax
	movl %eax, (BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS + 8)

	movw %cx, %bx
	cmpw $128, %bx
	jle bios_load_kernel_elf.loop
	movw $128, %bx
	jmp bios_load_kernel_elf.loop

bios_load_kernel_elf.done:
	// Store kernel ELF file size in boot info ELF size field.
	movl KERNEL_IMAGE_SIZE_MEM_ADDRESS, %eax
	movl %eax, X86_EARLY_BOOT_INFO_KERNEL_ELF_SIZE_ADDRESS_PHYS

	// Clear used registers.
	xorl %eax, %eax
	xorw %bx, %bx
	xorw %cx, %cx
	xorw %si, %si

	ret

bios_load_kernel_elf.fault:
	mov $bios_load_fail_str, %si
	call bios_say
bios_load_kernel_elf.forever:
	jmp bios_load_kernel_elf.forever

#endif

#include "bios_say.S"

e820_unsupported_str:
	.asciz "panic: e820 call unsupported!"

e820_call_failed_str:
	.asciz "panic: e820 call failed!"

e820_unexpected_length_str:
	.asciz "panic: e820 call unexpected entry length!"

bios_load_fail_str:
	.asciz "panic: BIOS ELF image load failed!"

.code32

start32:
	// It seems interrupts _can_ be re-established as we transition modes,
	// so to be sure clear them again.
	cli

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
	movl $(X86_PAGE_FLAG_DEFAULT | X86_EARLY_PUD_DIRECT0), X86_EARLY_PGD
	// PGE for VA X86_KERNEL_DIRECT_MAP_BASE -> PUD at 0x3000.
	movl $(X86_PAGE_FLAG_DEFAULT | X86_EARLY_PUD_DIRECT_MAP), (X86_EARLY_PGD + 8 * X86_KERNEL_DIRECT_MAP_BASE_PGD_OFFSET)
	// PGE for VA X86_KERNEL_ELF_BASE -> PUD at 0x4000.
	movl $(X86_PAGE_FLAG_DEFAULT | X86_EARLY_PUD_KERNEL_ELF), (X86_EARLY_PGD + 8 * X86_KERNEL_ELF_BASE_PGD_OFFSET)

	// Add 'gigantic' PUDs pointing at PA 0.
	movl $(X86_PAGE_FLAG_DEFAULT | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), X86_EARLY_PUD_DIRECT0
	movl $(X86_PAGE_FLAG_DEFAULT | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), X86_EARLY_PUD_DIRECT_MAP
	movl $(X86_PAGE_FLAG_DEFAULT | X86_PUD_FLAG_1GIB_PAGE_SIZE | 0), X86_EARLY_PUD_KERNEL_ELF

	// Assign PGD to CR3.
	movl $X86_EARLY_PGD, %edi
	movl %edi, %cr3

	// Enable PAE addressing (requied for long mode) and the ability to set
	// the global page table flag.
	movl %cr4, %eax
	orl $X86_CR4_INIT_FLAGS, %eax
	movl %eax, %cr4

	// Enable long mode.
	movl $X86_MFR_EFER, %ecx
	rdmsr
	orl $X86_MFR_EFER_INIT_FLAGS, %eax
	wrmsr

	// Enable paging.
	movl %cr0, %eax
	orl $X86_CR0_PAGED_MODE, %eax
	movl %eax, %cr0

	// Jump into long mode.
	ljmpl $GDT_SEGMENT_CODE64, $start64

.code64
start64:
	// It seems interrupts _can_ be re-established as we transition modes,
	// so to be sure clear them again.
	cli

	movw $GDT_SEGMENT_DATA, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	xorw %ax, %ax
	movw %ax, %fs
	movw %ax, %gs

	call retrieve_reset_apic

	// Setup stack.
	movq $X86_KERNEL_STACK_ADDRESS, %rsp
	// Call the kernel ELF loader.
	call load

	// If we reach here, something has gone wrong!
start64.forever:
	jmp start64.forever

retrieve_reset_apic:
	// Obtain APIC base address.
	movl $X86_IA32_APIC_BASE_MSR, %ecx
	rdmsr
	// Page align.
	shrq $12, %rax
	shlq $12, %rax
	// Limit to 36 bits (RDX stores higher bits).
	orq $0xf, %rdx

	// Copy RAX -> RCX, RDX -> RBX
	movq %rax, %rcx
	movq %rdx, %rbx

	// Store in early boot info.
	shlq $32, %rdx
	orq %rdx, %rax
	mov %rax, X86_EARLY_BOOT_INFO_APIC_BASE_ADDRESS_PHYS

	// Reset APIC base address to ensure set.
	mov %rcx, %rax
	mov %rbx, %rdx
	movl $X86_IA32_APIC_BASE_MSR_SET_ENABLE, %ecx
	wrmsr

	// Clear clobbered registers.
	xorq %rax, %rax
	xorq %rbx, %rbx
	xorq %rcx, %rcx
	xorq %rdx, %rdx

	ret

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
