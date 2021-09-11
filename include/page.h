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
 * |                                                      |||||||||
 * |- (nx) non-executable             63                  |||||||||
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
// Mask for bits in page size.
#define PAGE_MASK (PAGE_SIZE - 1)

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
// Aggregated page flags:
#define PAGE_FLAG_DEFAULT (PAGE_FLAG_PRESENT | PAGE_FLAG_RW)
#define PAGE_FLAG_KERNEL (PAGE_FLAG_DEFAULT | PAGE_FLAG_GLOBAL)
#define PAGE_FLAG_DEFAULT_DATA (PAGE_FLAG_DEFAULT | PAGE_FLAG_NX)

// The number of page table entries for each page table.
#define NUM_PAGE_TABLE_ENTRIES (512)

// Physical/virtual address types.
TYPE_WRAP(physaddr_t, uint64_t);
TYPE_WRAP(virtaddr_t, uint64_t);

// Represents a 'page frame number', essentially the index of a page in an array
// of pages, equal to the physical address shifted by PAGE_SHIFT.
TYPE_WRAP(pfn_t, uint64_t);

// Physical addresses for page tables.
TYPE_WRAP(pgdaddr_t, uint64_t);
TYPE_WRAP(pudaddr_t, uint64_t);
TYPE_WRAP(pmdaddr_t, uint64_t);
TYPE_WRAP(ptdaddr_t, uint64_t);

// Page table entry types.
TYPE_WRAP(pgde_t, uint64_t);
TYPE_WRAP(pude_t, uint64_t);
TYPE_WRAP(pmde_t, uint64_t);
TYPE_WRAP(ptde_t, uint64_t);

// Represents allocators for each page level.
struct page_allocators {
	pudaddr_t (*pud)(void);
	pmdaddr_t (*pmd)(void);
	ptdaddr_t (*ptd)(void);
	physaddr_t (*data)(void);
};

// Convert bytes to the number of pages required to hold them.
static inline uint64_t bytes_to_pages(uint64_t bytes)
{
	return ALIGN_UP(bytes, PAGE_SIZE) >> PAGE_SHIFT;
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

// Get pointer for virtual address (via direct mapping) from page table
// addresses.
#define GEN_PAGE_TO_VIRT_PTR(pagelevel)                                     \
	static inline void *pagelevel##_to_virt_ptr(pagelevel##addr_t addr) \
	{                                                                   \
		physaddr_t pa = {addr.x};                                   \
		return phys_to_virt_ptr(pa);                                \
	}
GEN_PAGE_TO_VIRT_PTR(pgd);
GEN_PAGE_TO_VIRT_PTR(pud);
GEN_PAGE_TO_VIRT_PTR(pmd);
GEN_PAGE_TO_VIRT_PTR(ptd);
#undef GEN_PAGE_TO_VIRT_PTR

// Get physical address from virtual address.
static inline physaddr_t virt_to_phys(virtaddr_t va)
{
	// Determine whether the VA is a direct mapping or part of the kernel
	// ELF.
	uint64_t offset;
	if (va.x > KERNEL_ELF_ADDRESS)
		offset = KERNEL_ELF_ADDRESS;
	else
		offset = KERNEL_DIRECT_MAP_BASE;

	physaddr_t pa = {va.x - offset};
	return pa;
}

// Get physical address from (kernel!) pointer.
static inline physaddr_t virt_ptr_to_phys(void *ptr)
{
	virtaddr_t va = {(uint64_t)ptr};
	return virt_to_phys(va);
}

// Zero a page of memory at the specified physical address.
static inline void zero_page(physaddr_t pa)
{
	memset(phys_to_virt_ptr(pa), 0, PAGE_SIZE);
}

// Convert a physical address to a page frame number.
static inline pfn_t pa_to_pfn(physaddr_t pa)
{
	pfn_t pfn = {pa.x >> PAGE_SHIFT};
	return pfn;
}

// Convert a physical address to a page frame number.
static inline physaddr_t pfn_to_pa(pfn_t pfn)
{
	physaddr_t pa = {pfn.x << PAGE_SHIFT};
	return pa;
}

// Return the physical address of the page after this PA.
static inline physaddr_t pa_next_page(physaddr_t pa)
{
	pfn_t pfn = pa_to_pfn(pa);
	pfn.x++;
	return pfn_to_pa(pfn);
}

// Return the physical address of the page before this PA. Doesn't check for
// underflow.
static inline physaddr_t pa_prev_page(physaddr_t pa)
{
	pfn_t pfn = pa_to_pfn(pa);
	pfn.x--;
	return pfn_to_pa(pfn);
}

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

// Determine whether PGDE is present.
static inline bool pgde_present(pgde_t pgde)
{
	return IS_SET(pgde.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PUDE is present.
static inline bool pude_present(pude_t pude)
{
	return IS_SET(pude.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PMDE is present.
static inline bool pmde_present(pmde_t pmde)
{
	return IS_SET(pmde.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PTDE is present.
static inline bool ptde_present(ptde_t ptde)
{
	return IS_SET(ptde.x, PAGE_FLAG_PRESENT_BIT);
}

// Retrieve pointer to PGD entry at specific index.
static inline pgde_t *pgde_at(pgdaddr_t pgd, uint64_t index)
{
	uint64_t *ptr = pgd_to_virt_ptr(pgd);
	return (pgde_t *)&ptr[index];
}

// Retrieve pointer to PUD entry at specific index.
static inline pude_t *pude_at(pudaddr_t pud, uint64_t index)
{
	uint64_t *ptr = pud_to_virt_ptr(pud);
	return (pude_t *)&ptr[index];
}

// Retrieve pointer to PMD entry at specific index.
static inline pmde_t *pmde_at(pmdaddr_t pmd, uint64_t index)
{
	uint64_t *ptr = pmd_to_virt_ptr(pmd);
	return (pmde_t *)&ptr[index];
}

// Retrieve pointer to PTD entry at specific index.
static inline ptde_t *ptde_at(ptdaddr_t ptd, uint64_t index)
{
	uint64_t *ptr = ptd_to_virt_ptr(ptd);
	return (ptde_t *)&ptr[index];
}

// Assign a PUD to a PGDE, set read/write and present.
static inline void assign_pud(pgdaddr_t pgd, uint64_t index, pudaddr_t pud)
{
	pgde_at(pgd, index)->x = (pud.x & ~PAGE_MASK) | PAGE_FLAG_DEFAULT;
}

// Assign a PMD to a PUDE, set read/write and present.
static inline void assign_pmd(pudaddr_t pud, uint64_t index, pmdaddr_t pmd)
{
	pude_at(pud, index)->x = (pmd.x & ~PAGE_MASK) | PAGE_FLAG_DEFAULT;
}

// Assign a PTD to a PMDE, set read/write and present.
static inline void assign_ptd(pmdaddr_t pmd, uint64_t index, ptdaddr_t ptd)
{
	pmde_at(pmd, index)->x = (ptd.x & ~PAGE_MASK) | PAGE_FLAG_DEFAULT;
}

// Map a range of virtual addresses from [start_va, start_va + num_pages) to
// [start_pa, start_pa + num_pages) under PGD and allocating new pages as
// required using alloc functions. Returns the number of pages allocated.
uint64_t _map_page_range(pgdaddr_t pgd, virtaddr_t start_va,
			 physaddr_t start_pa, uint64_t num_pages,
			 struct page_allocators *alloc);
