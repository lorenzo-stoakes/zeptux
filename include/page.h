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
#define PAGE_SIZE BIT_MASK(PAGE_SHIFT)
// Mask for bits in page size.
#define PAGE_MASK BIT_MASK_BELOW(PAGE_SHIFT)

// The numbers of meaningful bits in a physical address.
#define PHYS_ADDR_BITS X86_PHYS_ADDR_BITS
// A mask including the meaningful bits contained within a physical address.
#define PHYS_ADDR_MASK (BIT_MASK_BELOW(PHYS_ADDR_BITS))

// Page shifts for each page level.
#define PMD_SHIFT (X86_PMD_SHIFT)
#define PUD_SHIFT (X86_PUD_SHIFT)
#define PGD_SHIFT (X86_PGD_SHIFT)

// Assign page masks/sizes for larger page sizes specified at PUD/PMD level.
#define PGD_SIZE BIT_MASK(PGD_SHIFT)
#define PAGE_SIZE_1GIB BIT_MASK(PUD_SHIFT)
#define PAGE_SIZE_2MIB BIT_MASK(PMD_SHIFT)
#define PAGE_MASK_1GIB BIT_MASK_BELOW(PUD_SHIFT)
#define PAGE_MASK_2MIB BIT_MASK_BELOW(PMD_SHIFT)

// Masks for obtaining physical addresses from page table entries.
#define PAGE_TABLE_PHYS_ADDR_MASK (PHYS_ADDR_MASK & BIT_MASK_ABOVE(PAGE_SHIFT))
#define PAGE_TABLE_PHYS_ADDR_MASK_2MIB \
	(PHYS_ADDR_MASK & BIT_MASK_ABOVE(PMD_SHIFT))
#define PAGE_TABLE_PHYS_ADDR_MASK_1GIB \
	(PHYS_ADDR_MASK & BIT_MASK_ABOVE(PUD_SHIFT))

// Mask for each page directory index portion of a virtual address.
#define PAGE_DIR_INDEX_MASK BIT_MASK_BELOW(9)

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

// Mask for available page flags in a page table entry.
#define PAGE_TABLE_FLAG_MASK (BIT_MASK_BELOW(PAGE_SHIFT) | PAGE_FLAG_NX)

// The number of page table entries for each page table.
#define NUM_PAGE_TABLE_ENTRIES (512)

// The number of pages mapped by each page table level.
#define NUM_PAGES_PTD (512)		    //   2 MiB
#define NUM_PAGES_PMD (512 * NUM_PAGES_PTD) //   1 GiB
#define NUM_PAGES_PUD (512 * NUM_PAGES_PMD) // 512 GiB

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

	void NORETURN (*panic)(const char *fmt, ...);
};

// Represents flagging modes. The lower bits contain a discrete mapping mode,
// and the upper flags act as modifiers.
#define MAP_FLAG_MODE_BITS (10)
typedef enum {
	// Lower bits specify mode:
	MAP_KERNEL = 1,
	MAP_KERNEL_NOGLOBAL = 2,
	MAP_DEVICE = 3,
	// Upper bits specify modifiers:
	MAP_CODE =
		BIT_MASK(MAP_FLAG_MODE_BITS + 0), // We default to setting NX.
	MAP_READONLY = BIT_MASK(MAP_FLAG_MODE_BITS + 1),
} map_flags_t;
#define MAP_FLAG_MODE_MASK (BIT_MASK_BELOW(MAP_FLAG_MODE_BITS))

// Represents page table levels.
typedef enum {
	PGD,
	PUD,
	PMD,
	PTD,
} page_level_t;

// Convert map flags to DATA page flags.
static inline uint64_t map_flags_to_page_flags(map_flags_t map_flags)
{
	uint64_t flags = 0;
	switch (map_flags & MAP_FLAG_MODE_MASK) {
	case MAP_KERNEL:
		flags = PAGE_FLAG_KERNEL;
		break;
	case MAP_KERNEL_NOGLOBAL:
		flags = PAGE_FLAG_DEFAULT;
		break;
	case MAP_DEVICE:
		flags = PAGE_FLAG_DEFAULT | PAGE_FLAG_UNCACHED |
			PAGE_FLAG_WRITE_THROUGH;
		break;
	default:
		// TODO: Panic?
		return 0;
	}

	// We always set NX unless we are explicitly mapping code.
	if (!IS_MASK_SET(map_flags, MAP_CODE))
		flags |= PAGE_FLAG_NX;

	// Clear RW flag only if explicitly readonly.
	if (IS_MASK_SET(map_flags, MAP_READONLY))
		flags ^= PAGE_FLAG_RW;

	return flags;
}

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
	return addr.x & PAGE_MASK;
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

// Encodes a virtual address from page table indexes and data page offset. This
// assumes a 4 KiB page mapping. We don't check any of the parameters are within
// range so this assumes the caller is sane.
static inline virtaddr_t encode_virt(uint64_t pgde_ind, uint64_t pude_ind,
				     uint64_t pmde_ind, uint64_t ptde_ind,
				     uint64_t offset)
{
	virtaddr_t va = {(pgde_ind << PGD_SHIFT) | (pude_ind << PUD_SHIFT) |
			 (pmde_ind << PMD_SHIFT) | (ptde_ind << PAGE_SHIFT) |
			 offset};
	return va;
}

// Obtain the virtual address occupying the next PGDE index.
static inline virtaddr_t virt_next_pgde(virtaddr_t addr)
{
	virtaddr_t ret = {((addr.x >> PGD_SHIFT) + 1) << PGD_SHIFT};
	return ret;
}

// Obtain the virtual address occupying the next PUDE index.
static inline virtaddr_t virt_next_pude(virtaddr_t addr)
{
	// The carry will increment PGD index as appropriate.
	virtaddr_t ret = {((addr.x >> PUD_SHIFT) + 1) << PUD_SHIFT};
	return ret;
}

// Obtain the virtual address occupying the next PMDE index.
static inline virtaddr_t virt_next_pmde(virtaddr_t addr)
{
	// The carry will increment PGD, PUD indexes as appropriate.
	virtaddr_t ret = {((addr.x >> PMD_SHIFT) + 1) << PMD_SHIFT};
	return ret;
}

// Determine how many pages remaining before a new PGDE entry need be assigned.
static inline uint64_t virt_pgde_remaining_pages(virtaddr_t addr)
{
	// Clear 4 KiB data page offset.
	addr.x &= BIT_MASK_ABOVE(PAGE_SHIFT);
	virtaddr_t next = virt_next_pgde(addr);

	return bytes_to_pages(next.x - addr.x);
}

// Determine how many pages remaining before a new PUDE entry (+ possibly a new
// PGDE entry) need be assigned.
static inline uint64_t virt_pude_remaining_pages(virtaddr_t addr)
{
	// Clear 4 KiB data page offset.
	addr.x &= BIT_MASK_ABOVE(PAGE_SHIFT);
	virtaddr_t next = virt_next_pude(addr);

	return bytes_to_pages(next.x - addr.x);
}

// Determine how many pages remaining before a new PMDE entry (+ possibly a new
// PUDE, perhaps PGDE entry) need be assigned.
static inline uint64_t virt_pmde_remaining_pages(virtaddr_t addr)
{
	// Clear 4 KiB data page offset.
	addr.x &= BIT_MASK_ABOVE(PAGE_SHIFT);
	virtaddr_t next = virt_next_pmde(addr);

	return bytes_to_pages(next.x - addr.x);
}

// Offset an address by the specified number of 4 KiB pages. This IGNORES the
// data page offset, treating it as if it were zero.
#define GEN_OFFSET_PAGES(pagetype)                              \
	static inline pagetype##addr_t pagetype##_offset_pages( \
		pagetype##addr_t addr, uint64_t pages)          \
	{                                                       \
		uint64_t val = addr.x >> PAGE_SHIFT;            \
		val += pages;                                   \
		val <<= PAGE_SHIFT;                             \
		pagetype##addr_t ret = {val};                   \
		return ret;                                     \
	}
GEN_OFFSET_PAGES(virt);
GEN_OFFSET_PAGES(phys);

// Determine whether PGDE is present.
static inline bool pgde_present(pgde_t pgde)
{
	return IS_BIT_SET(pgde.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PUDE is present.
static inline bool pude_present(pude_t pude)
{
	return IS_BIT_SET(pude.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PMDE is present.
static inline bool pmde_present(pmde_t pmde)
{
	return IS_BIT_SET(pmde.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PTDE is present.
static inline bool ptde_present(ptde_t ptde)
{
	return IS_BIT_SET(ptde.x, PAGE_FLAG_PRESENT_BIT);
}

// Determine whether PUDE is referencing a 1 GiB data page.
static inline bool pude_1gib(pude_t pude)
{
	return IS_BIT_SET(pude.x, PAGE_FLAG_PSE_BIT);
}

// Determine whether PMDE is referencing a 2 MiB data page.
static inline bool pmde_2mib(pmde_t pmde)
{
	return IS_BIT_SET(pmde.x, PAGE_FLAG_PSE_BIT);
}

// Retrieve the PUD address referenced by a PGDE.
static inline pudaddr_t pgde_pud(pgde_t pgde)
{
	pudaddr_t pud = {pgde.x & PAGE_TABLE_PHYS_ADDR_MASK};
	return pud;
}

// Retrieve the PMD address reference by a PUDE.
static inline pmdaddr_t pude_pmd(pude_t pude)
{
	pmdaddr_t pmd = {pude.x & PAGE_TABLE_PHYS_ADDR_MASK};
	return pmd;
}

// Retrieve the 1GiB data page address referenced by a PUDE.
static inline physaddr_t pude_data_1gib(pude_t pude)
{
	physaddr_t pa = {pude.x & PAGE_TABLE_PHYS_ADDR_MASK_1GIB};
	return pa;
}

// Retrieve raw page table flags from specified 1 GiB PUDE.
static inline uint64_t pude_raw_flags_1gib(pude_t pude)
{
	return pude.x & PAGE_TABLE_FLAG_MASK;
}

// Retrieve the PTD address reference by a PMDE.
static inline ptdaddr_t pmde_ptd(pmde_t pmde)
{
	ptdaddr_t ptd = {pmde.x & PAGE_TABLE_PHYS_ADDR_MASK};
	return ptd;
}

// Retrieve the 2MiB data page address referenced by a PMDE.
static inline physaddr_t pmde_data_2mib(pmde_t pmde)
{
	physaddr_t pa = {pmde.x & PAGE_TABLE_PHYS_ADDR_MASK_2MIB};
	return pa;
}

// Retrieve raw page table flags from specified 2 MiB PMDE.
static inline uint64_t pmde_raw_flags_2mib(pmde_t pmde)
{
	return pmde.x & PAGE_TABLE_FLAG_MASK;
}

// Retrieve the 4KiB data page address referenced by a PTDE.
static inline physaddr_t ptde_data(ptde_t ptde)
{
	physaddr_t pa = {ptde.x & PAGE_TABLE_PHYS_ADDR_MASK};
	return pa;
}

// Retrieve raw page tables flags from specified PTDE.
static inline uint64_t ptde_raw_flags(ptde_t ptde)
{
	return ptde.x & PAGE_TABLE_FLAG_MASK;
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

// Assign a 1 GiB data page to a PUDE.
static inline void assign_data_1gib(pudaddr_t pud, uint64_t index,
				    physaddr_t pa, map_flags_t flags)
{
	pude_at(pud, index)->x = (pa.x & ~PAGE_MASK_1GIB) | PAGE_FLAG_PSE |
				 map_flags_to_page_flags(flags);
}

// Assign a PMD to a PUDE, set read/write and present.
static inline void assign_pmd(pudaddr_t pud, uint64_t index, pmdaddr_t pmd)
{
	pude_at(pud, index)->x = (pmd.x & ~PAGE_MASK) | PAGE_FLAG_DEFAULT;
}

// Assign a 2 MiB data page to a PMDE.
static inline void assign_data_2mib(pmdaddr_t pmd, uint64_t index,
				    physaddr_t pa, map_flags_t flags)
{
	pmde_at(pmd, index)->x = (pa.x & ~PAGE_MASK_2MIB) | PAGE_FLAG_PSE |
				 map_flags_to_page_flags(flags);
}

// Assign a PTD to a PMDE, set read/write and present.
static inline void assign_ptd(pmdaddr_t pmd, uint64_t index, ptdaddr_t ptd)
{
	pmde_at(pmd, index)->x = (ptd.x & ~PAGE_MASK) | PAGE_FLAG_DEFAULT;
}

// Assign a 4 KiB data page to a PTDE.
static inline void assign_data(ptdaddr_t ptd, uint64_t index, physaddr_t pa,
			       map_flags_t flags)
{
	ptde_at(ptd, index)->x = (pa.x & ~PAGE_MASK) |
				 map_flags_to_page_flags(flags);
}

// Map a range of virtual addresses from [start_va, start_va + num_pages) to
// [start_pa, start_pa + num_pages) under PGD and allocating new pages as
// required using alloc functions. Returns the number of pages allocated.
uint64_t _map_page_range(pgdaddr_t pgd, virtaddr_t start_va,
			 physaddr_t start_pa, int64_t num_pages,
			 map_flags_t flags, struct page_allocators *alloc);

// Walk page tables to retrieve the raw arch page flags for the specified VA in
// the specified PGD. Use `alloc` to panic.
uint64_t _walk_virt_to_raw_flags(pgdaddr_t pgd, virtaddr_t va,
				 struct page_allocators *alloc);

// Walk page tables to retrieve PA for specified VA from specific PGD. use
// `alloc` to panic.
physaddr_t _walk_virt_to_phys(pgdaddr_t pgd, virtaddr_t va,
			      struct page_allocators *alloc);
