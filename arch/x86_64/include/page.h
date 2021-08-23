#pragma once

#include "macros.h"
#include "mem.h"
#include "types.h"
#include "x86-consts.h"

/*
 * We denote 4-level intel page tables at each level as follows:
 *
 * PGD - Page Global Directory
 * PUD - Page Upper Directory
 * PMD - Page Middle Directory
 * PTD - Page Table Directory
 *
 * Each table consists of 512 8-byte entries (denoted PxDE).
 *
 * A virtual address indexes into each of the page tables:
 *
 * xxxxxxxxxxxxxxxxx                   48 bits
 * <--------------><---------------------------------------------->
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 *                 [  PGD  ][  PUD  ][  PMD  ][  PTE  ][  OFFSET  ]
 *                         |        |        |        |
 *                         |        |        |        |------------- PAGE_SHIFT (12)
 *                         |        |        |---------------------- PMD_SHIFT  (21)
 *                         |        |------------------------------- PUD_SHIFT  (30)
 *                         |---------------------------------------- PGD_SHIFT  (39)
 *
 * A page table entry contains the physical address of the page table it refers
 * to as well as a number of flags.  As all page tables must be page-aligned,
 * the lower 12 bits are available for flags. In addition there are higher bits
 * that will not form part of a 48-bit physical adddress which are either
 * reserved or contain other flags.
 *
 * The page table entries are structured as follows: (See Intel Volume 3A, part
 * 1, Figure 4-11)
 *
 * NOTE: In the below 'x' denotes a flag that can be set. These are defined in
 * x86-consts.
 *
 * CR3 entry:
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * [   reserved   ][      page-aligned PA of PGD      ][r]    xx
 *
 * PGDE:
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * x[   reserved  ][      page-aligned PA of PUD      ][r]   xxxxxx
 *
 * PUDE (1 GiB page):
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * x[   reserved  ][ PA / data page ][     reserved      ]x1xxxxxxx
 *
 * PUDE (<1 GiB page):
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * x[   reserved  ][      page-aligned PA of PMD      ][r] 0 xxxxxx
 *
 * PMDE (2 MiB page):
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * x[   reserved  ][     PA / data page      ][ reserved ]x1xxxxxxx
 *
 * PMDE (4 KiB page):
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * x[   reserved  ][      page-aligned PA of PTD      ][r] 0 xxxxxx
 *
 * PTDE:
 *    6         5         4         3         2         1
 * ---|---------|---------|---------|---------|---------|----------
 * 3210987654321098765432109876543210987654321098765432109876543210
 * x[   reserved  ][   page-aligned PA of data page   ][r]x0xxxxxxx
 *                                                        |||||||||
 *                                     8          global -|||||||||
 *                                     7             PSE --||||||||
 *                                     6           dirty ---|||||||
 *                                     5        accessed ----||||||
 *                                     4        uncached -----|||||
 *                                     3   write through ------||||
 *                                     2            user -------|||
 *                                     1              RW --------||
 *                                     0         present ---------|
 */

// Number of bits contained within default memory page size.
#define PAGE_SHIFT (X86_PAGE_SHIFT)
// The default memory page size.
#define PAGE_SIZE (1UL << PAGE_SHIFT)

// Page shifts for each page level.
#define PMD_SHIFT (X86_PMD_SHIFT)
#define PUD_SHIFT (X86_PUD_SHIFT)
#define PGD_SHIFT (X86_PGD_SHIFT)

// Mask for each page directory index portion of a virtual address.
#define PAGE_DIR_INDEX_MASK ((1UL << 9) - 1)

// Mask for data offset portion of a virtual address.
#define PAGE_DATA_OFFSET_MASK ((1UL << PAGE_SHIFT) - 1)

// Import X86 page flag bits:
#define PAGE_FLAG_PRESENT_BIT X86_PAGE_FLAG_PRESENT_BIT
#define PAGE_FLAG_RW_BIT X86_PAGE_FLAG_RW_BIT
#define PAGE_FLAG_USER_BIT X86_PAGE_FLAG_USER_BIT
#define PAGE_FLAG_WRITE_THROUGH_BIT X86_PAGE_FLAG_WRITE_THROUGH_BIT
#define PAGE_FLAG_UNCACHED_BIT X86_PAGE_FLAG_UNCACHED_BIT
#define PAGE_FLAG_ACCESSED_BIT X86_PAGE_FLAG_ACCESSED_BIT
#define PAGE_FLAG_DIRTY_BIT X86_PAGE_FLAG_DIRTY_BIT
#define PAGE_FLAG_PSE_BIT X86_PAGE_FLAG_PSE_BIT
#define PAGE_FLAG_GLOBAL_BIT X86_PAGE_FLAG_GLOBAL_BIT
#define PAGE_FLAG_NX_BIT X86_PAGE_FLAG_NX_BIT
// Import X86 page flags:
#define PAGE_FLAG_PRESENT X86_PAGE_FLAG_PRESENT
#define PAGE_FLAG_RW X86_PAGE_FLAG_RW
#define PAGE_FLAG_USER X86_PAGE_FLAG_USER
#define PAGE_FLAG_WRITE_THROUGH X86_PAGE_FLAG_WRITE_THROUGH
#define PAGE_FLAG_UNCACHED X86_PAGE_FLAG_UNCACHED
#define PAGE_FLAG_ACCESSED X86_PAGE_FLAG_ACCESSED
#define PAGE_FLAG_DIRTY X86_PAGE_FLAG_DIRTY
#define PAGE_FLAG_PSE X86_PAGE_FLAG_PSE
#define PAGE_FLAG_GLOBAL X86_PAGE_FLAG_GLOBAL
#define PAGE_FLAG_NX X86_PAGE_FLAG_NX

// Physical/virtual address types.
TYPE_WRAP(physaddr_t, uint64_t);
TYPE_WRAP(virtaddr_t, uint64_t);

// Page table entry types.
TYPE_WRAP(pgde_t, uint64_t);
TYPE_WRAP(pude_t, uint64_t);
TYPE_WRAP(pmde_t, uint64_t);
TYPE_WRAP(ptde_t, uint64_t);

// Get data offset from virtual address.
static inline uint64_t virt_data_offset(virtaddr_t addr)
{
	return addr.x & PAGE_DATA_OFFSET_MASK;
}

// Get PTDE offset from virtual address.
static inline uint64_t virt_ptde_index(virtaddr_t addr)
{
	return (addr.x >> PAGE_SHIFT) & PAGE_DIR_INDEX_MASK;
}

// Get PMDE offset from virtual address.
static inline uint64_t virt_pmde_index(virtaddr_t addr)
{
	return (addr.x >> PMD_SHIFT) & PAGE_DIR_INDEX_MASK;
}

// Get PUDE offset from virtual address.
static inline uint64_t virt_pude_index(virtaddr_t addr)
{
	return (addr.x >> PUD_SHIFT) & PAGE_DIR_INDEX_MASK;
}

// Get PGDE offset from virtual address.
static inline uint64_t virt_pgde_index(virtaddr_t addr)
{
	return (addr.x >> PGD_SHIFT) & PAGE_DIR_INDEX_MASK;
}

// Get virtual address (via direct mapping) from physical address.
static inline virtaddr_t phys_to_virt(physaddr_t addr)
{
	virtaddr_t va = {KERNEL_DIRECT_MAP_BASE + addr.x};
	return va;
}

// Get pointer for virtual address (via direct mapping) from physical
// address.
static inline void *phys_to_virt_ptr(physaddr_t addr)
{
	return (void *)phys_to_virt(addr).x;
}

// Get physical address from virtual address.
static inline physaddr_t virt_to_phys(virtaddr_t addr)
{
	// Determine whether the VA is a direct mapping or part of the kernel
	// ELF.
	uint64_t offset;
	if (addr.x > KERNEL_ELF_ADDRESS)
		offset = KERNEL_ELF_ADDRESS;
	else
		offset = KERNEL_DIRECT_MAP_BASE;

	physaddr_t pa = {addr.x - offset};
	return pa;
}