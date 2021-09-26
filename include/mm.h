#pragma once

// See docs/memmap.md for more information on the physical and virtual memory
// maps as provided by hardware and assigned by zeptux.

#include "atomic.h"
#include "compiler.h"
#include "list.h"
#include "mm_defs.h"
#include "page.h"
#include "spinlock.h"
#include "types.h"

// Represents the type of a physical memory block.
typedef enum physblock_type {
	PHYSBLOCK_UNALLOC = 0,
	PHYSBLOCK_TAIL = 1,
	PHYSBLOCK_PAGETABLE = 2,
	PHYSBLOCK_PHYSBLOCK = 3,
	PHYSBLOCK_KERNEL = 4,
	PHYSBLOCK_USER = 5,
	PHYSBLOCK_MOVABLE = 1 << 10,
	PHYSBLOCK_PINNED = 1 << 11,
} physblock_type_t;

// Describes a 'block' of physical memory of size 2^order pages.
struct physblock {
	// If a physblock comprises > 1 page and a PA relates to a page other
	// than the first, we store all relevant physblock data in only the
	// first 'head' page, and not the remaining 'tail' pages. We set
	// head_offset in each tail page to indicate where the head is.
	uint8_t head_offset;
	uint8_t order; // 2^order pages in this physblock.

	physblock_type_t type;

	struct list_node node;

	uint32_t refcount;
	spinlock_t lock; // All fields are protected by this lock.
};
// We assign max 1 TiB of physblock descriptors in the memory map, plus it's
// useful to keep it a cacheline size.
static_assert(sizeof(struct physblock) <= 64);
// A page must be evenly divisible into physblocks.
// TODO: We have to explicitly assume PAGE_SIZE == 4096 here to avoid a
// dependency cycle.
static_assert(sizeof(struct physblock) * (4096 / sizeof(struct physblock)) ==
	      4096);

// Represents per-order statistics.
struct phys_alloc_order_stats {
	// Number of pages each comprising 2^order 4 KiB pages.
	uint64_t num_pages;
	uint64_t num_free_pages;
};

// Represents overall statistics relating to the physical allocator.
struct phys_alloc_stats {
	// These stats include pages comprising compound pages.
	uint64_t num_4k_pages;
	uint64_t num_free_4k_pages;
	uint64_t num_pagetable_pages;
	uint64_t num_physblock_pages;
	// Per-order statistics.
	struct phys_alloc_order_stats order[MAX_ORDER];
};

// Represents physical allocator state.
struct phys_alloc_state {
	struct list free_lists[MAX_ORDER];
	struct phys_alloc_stats stats;

	spinlock_t lock;
};

// The root kernel PGD.
extern pgdaddr_t kernel_root_pgd;

// Initialise the physical allocator state, `ptr` points to an early allocated
// memory page. Called during early memory initialisation.
void phys_alloc_init_state(void *ptr);

// Gets the physical allocator state.
struct phys_alloc_state *phys_get_alloc_state(void);

// Free a physical page into the allocator.
void phys_free(physaddr_t pa);

// Obtain a pointer to a physblock from a PFN without either locking it or
// determining if the block is a tail entry.
static inline struct physblock *_pfn_to_physblock_raw(pfn_t pfn)
{
	virtaddr_t va = {KERNEL_MEM_MAP_ADDRESS +
			 pfn.x * sizeof(struct physblock)};

	return (struct physblock *)virt_to_ptr(va);
}

// Obtain a pointer to a physblock from a PFN without determining if the block
// is a tail entry.
static inline struct physblock *_pfn_to_physblock_raw_lock(pfn_t pfn)
{
	struct physblock *block = _pfn_to_physblock_raw(pfn);
	spinlock_acquire(&block->lock);

	return block;
}

// Obtain the HEAD physblock entry for this TAIL physblock.
// ASSUMES: physblock lock is held.
static inline struct physblock *physblock_tail_to_head(struct physblock *block,
						       pfn_t pfn)
{
	uint8_t offset = block->head_offset;
	spinlock_release(&block->lock);

	pfn.x -= offset;
	return _pfn_to_physblock_raw_lock(pfn);
}

// Obtain a pointer to a physblock, obtains head physblock if points to a tail
// entry and ACQUIRES spinlock on the physblock which the consumer must release
// after use.
static inline struct physblock *phys_to_physblock_lock(physaddr_t pa)
{
	pfn_t pfn = phys_to_pfn(pa);
	struct physblock *block = _pfn_to_physblock_raw_lock(pfn);

	return block->head_offset == 0 ? block
				       : physblock_tail_to_head(block, pfn);
}
