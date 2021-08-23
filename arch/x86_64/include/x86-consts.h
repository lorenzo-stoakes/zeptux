#pragma once

// CPUID flags:
#define X86_LONGMODE_FLAG (1UL << 29)

// Global Descriptor Table (GDT) entry flags:
#define X86_GDTE_FLAG_4K_GRANULARITY (1UL << 3)
#define X86_GDTE_FLAG_1B_GRANULARITY (0 << 3)
// This one is weird - in a 64-bit code segment this must be unset, but we also
// set this for our data segment in long mode.
#define X86_GDTE_FLAG_32BIT_PROTECTED (1UL << 2)
#define X86_GDTE_FLAG_16BIT_PROTECTED (0 << 2)
#define X86_GDTE_FLAG_64BIT_CODE (1UL << 1)
#define X86_GDTE_ACCESS_PRESENT (1UL << 7)
#define X86_GDTE_ACCESS_RING0 (0)
#define X86_GDTE_ACCESS_RING1 (1UL << 5)
#define X86_GDTE_ACCESS_RING2 (2UL << 5)
#define X86_GDTE_ACCESS_RING3 (3UL << 5)
#define X86_GDTE_ACCESS_SYSTEM (0 << 5)
#define X86_GDTE_ACCESS_NONSYS (1UL << 4)
#define X86_GDTE_ACCESS_EXEC (1UL << 3)
#define X86_GDTE_ACCESS_DATA_GROW_UP (1UL << 2)
#define X86_GDTE_ACCESS_DATA_GROW_DOWN (0 << 2)
#define X86_GDTE_ACCESS_RING_CONFORM (1UL << 2)
#define X86_GDTE_ACCESS_RING_NOCONFORM (0 << 2)
#define X86_GDTE_ACCESS_RW (1UL << 1)
#define X86_GDTE_ACCESS_ACCESSED (1UL << 0)

// Control register flags
#define X86_CR0_PROTECTED_MODE (1UL << 0)
#define X86_CR0_PAGED_MODE (1UL << 31)
#define X86_CR4_PAE (1UL << 5)

#define X86_MFR_EFER (0xc0000080UL)

#define X86_MFR_EFER_LME (1UL << 8) // Enable long mode.

// Page table constants:

#define X86_PAGE_FLAG_PRESENT_BIT (0)
#define X86_PAGE_FLAG_RW_BIT (1)
#define X86_PAGE_FLAG_USER_BIT (2)
#define X86_PAGE_FLAG_WRITE_THROUGH_BIT (3)
#define X86_PAGE_FLAG_CACHE_DISABLED_BIT (4)
#define X86_PAGE_FLAG_ACCESSED_BIT (5)
#define X86_PAGE_FLAG_DIRTY_BIT (6)
#define X86_PAGE_FLAG_PSE_BIT (7)
#define X86_PAGE_FLAG_GLOBAL_BIT (8)
#define X86_PAGE_FLAG_NX_BIT (63)

#define X86_PAGE_FLAG_PRESENT (1UL << X86_PAGE_FLAG_PRESENT_BIT)
#define X86_PAGE_FLAG_RW (1UL << X86_PAGE_FLAG_RW_BIT)
#define X86_PAGE_FLAG_USER (1UL << X86_PAGE_FLAG_USER_BIT)
#define X86_PAGE_FLAG_WRITE_THROUGH (1UL << X86_PAGE_FLAG_WRITE_THROUGH_BIT)
#define X86_PAGE_FLAG_CACHE_DISABLED (1UL << X86_PAGE_FLAG_CACHE_DISABLED_BIT)
#define X86_PAGE_FLAG_ACCESSED (1UL << X86_PAGE_FLAG_ACCESSED_BIT)
#define X86_PAGE_FLAG_DIRTY (1UL << X86_PAGE_FLAG_DIRTY_BIT)
#define X86_PAGE_FLAG_PSE (1UL << X86_PAGE_FLAG_PSE_BIT)
#define X86_PAGE_FLAG_GLOBAL (1UL << X86_PAGE_FLAG_GLOBAL_BIT)
#define X86_PAGE_FLAG_NX (1UL << X86_PAGE_FLAG_NX_BIT)

#define X86_PAGE_FLAG_KERNEL \
	(X86_PAGE_FLAG_PRESENT | X86_PAGE_FLAG_RW | X86_PAGE_FLAG_GLOBAL)

// We establish larger page sizes by setting the PSE flag.
#define X86_PUD_FLAG_1GIB_PAGE_SIZE (X86_PAGE_FLAG_PSE)
#define X86_PMD_FLAG_2MIB_PAGE_SIZE (X86_PAGE_FLAG_PSE)

// Assuming 4-level page tables we have 48 bits to play with. x86 requires that
// the MSB is set for all higher bits, thus the very first address which sets
// higher bits to 1 is 0xffff800000000000 which is at the exact halfway mark of
// virtual memory. This makes it the ideal place to divide between userland and
// kernel memory.

//    6         5         4         3         2         1
// 3210987654321098765432109876543210987654321098765432109876543210
// 1111111111111111100000000000000000000000000000000000000000000000
// ................100000000000000000000000000000000000000000000000
#define X86_KERNEL_BASE (0xffff800000000000UL)

// Immediately at the beginning of kernel memory we provide a 64 TiB direct
// mapping of physical memory. This is SUPER convenient and takes advantage of
// the HUGE 64-bit address space.
#define X86_KERNEL_DIRECT_MAP_BASE (X86_KERNEL_BASE)
#define X86_KERNEL_DIRECT_MAP_BASE_PGD_OFFSET (256)

// We place this 64 TiB after the direct physical mapping. This is a direct
// memory mapping so the ELF is actually loaded at X86_KERNEL_ELF_BASE + x86_KERNEL_ELF_OFFSET.
#define X86_KERNEL_ELF_BASE (0xffffc00000000000UL)
#define X86_KERNEL_ELF_BASE_PGD_OFFSET (384)
// in x86 physical memory map, this is where extended memory lives so is safe to
// write to.
#define X86_KERNEL_ELF_OFFSET (0x1000000UL)
#define X86_KERNEL_ELF_ADDRESS (X86_KERNEL_ELF_BASE + X86_KERNEL_ELF_OFFSET)
// We specify that the kernel text section comes first and is offset by 0x1000
// so if booting from boot sector we can quickly load the ELF image direct and
// jump to it as headers will never exceed 1 page in size.
#define X86_KERNEL_TEXT (X86_KERNEL_ELF_ADDRESS + 0x1000UL)

#define X86_EARLY_PGD (0x1000)
#define X86_EARLY_PUD_DIRECT0 (0x2000)
#define X86_EARLY_PUD_DIRECT_MAP (0x3000)
#define X86_EARLY_PUD_KERNEL_ELF (0x4000)

// Place very early stack at start of boot sector.
#define X86_EARLY_INIT_STACK (0x7c00)
// Place the kernel stack at the top of conventional memory. Use the direct
// memory map to reference it.
#define X86_KERNEL_INIT_STACK (X86_KERNEL_DIRECT_MAP_BASE + 0x80000)
// Place the early boot info struct into lower part of conventional
// memory below boot sector.
#define X86_EARLY_BOOT_INFO_ADDRESS_PHYS (0x6000)
#define X86_EARLY_BOOT_INFO_E820_ENTRY_COUNT_ADDRESS_PHYS \
	(X86_EARLY_BOOT_INFO_ADDRESS_PHYS + 16)
#define X86_EARLY_BOOT_INFO_E820_ENTRY_ARRAY_ADDRESS_PHYS \
	(X86_EARLY_BOOT_INFO_ADDRESS_PHYS + 24)
#define X86_EARLY_BOOT_INFO_ADDRESS \
	(X86_KERNEL_DIRECT_MAP_BASE + X86_EARLY_BOOT_INFO_ADDRESS_PHYS)

// The magic number used by E820 BIOS operations to assert that you want to
// write E820 entries to memory.
#define X86_E820_MAGIC_NUMBER (0x534d4150)

// Offset into physical memory where we can output video characters to be
// displayed on the monitor.
#define X86_VIDEO_MEMORY_ADDRESS_PHYS (0xb8000)

// Offset into physical memory where we can store a buffer to copy into video
// memory. We use the memory above the original boot sector which is no longer
// needed.
#define X86_EARLY_VIDEO_BUFFER_ADDRESS_PHYS (0x7e00)

// The default page size for x86-64 is 4 KiB.
#define X86_PAGE_SHIFT (12)
