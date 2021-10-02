#pragma once

// See docs/memmap.md for more information on the physical and virtual memory
// maps as provided by hardware and assigned by zeptux.

// We implement a buddy allocator, see
// https://en.wikipedia.org/wiki/Buddy_memory_allocation

#include "atomic.h"
#include "compiler.h"
#include "list.h"
#include "mm_defs.h"
#include "page.h"
#include "spinlock.h"
#include "types.h"

// Represents the type of a physical memory block.
typedef enum physblock_type {
	PHYSBLOCK_UNMANAGED = 0, // Unallocated but not managed by phys alloc.
	PHYSBLOCK_FREE = 1,
	PHYSBLOCK_TAIL = 2,
	PHYSBLOCK_PAGETABLE = 3,
	PHYSBLOCK_PHYSBLOCK = 4,
	PHYSBLOCK_KERNEL = 5,
	PHYSBLOCK_USER = 6,
	PHYSBLOCK_TYPE_MASK = BIT_MASK_BELOW(10),
	PHYSBLOCK_MOVABLE = 1 << 10,
	PHYSBLOCK_PINNED = 1 << 11,
} physblock_type_t;

typedef enum {
	ALLOC_KERNEL = 1,
	ALLOC_USER = 2,
	ALLOC_PAGETABLE = 3,
	ALLOC_PHYSBLOCK = 4,
	ALLOC_TYPE_MASK = BIT_MASK_BELOW(10),
	ALLOC_MOVABLE = 1 << 10,
	ALLOC_PINNED = 1 << 11,
} alloc_flags_t;

// Describes a 'block' of physical memory of size 2^order pages.
struct physblock {
	// If a physblock comprises > 1 page and a PA relates to a page other
	// than the first, we store all relevant physblock data in only the
	// first 'head' page, and not the remaining 'tail' pages. We set
	// head_offset in each tail page to indicate where the head is.
	uint16_t head_offset;
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
	struct phys_alloc_order_stats order[MAX_ORDER + 1];
};

// Represents a span of available physical memory.
struct phys_alloc_span {
	pfn_t start_pfn;
	uint64_t num_pages;
};

// Represents physical allocator state.
struct phys_alloc_state {
	struct list free_lists[MAX_ORDER + 1];
	struct phys_alloc_stats stats;

	spinlock_t lock;

	// This can be accessed without a lock as they will be written once and
	// never changed (we don't support hot swapping).
	uint64_t num_spans;
	struct phys_alloc_span spans[0];
};

// The root kernel PGD.
extern pgdaddr_t kernel_root_pgd;

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
static inline struct physblock *pfn_to_physblock_lock(pfn_t pfn)
{
	struct physblock *block = _pfn_to_physblock_raw_lock(pfn);

	return block->head_offset == 0 ? block
				       : physblock_tail_to_head(block, pfn);
}

// Obtain a pointer to a physblock, obtains head physblock if points to a tail
// entry and ACQUIRES spinlock on the physblock which the consumer must release
// after use.
static inline struct physblock *phys_to_physblock_lock(physaddr_t pa)
{
	pfn_t pfn = phys_to_pfn(pa);
	return pfn_to_physblock_lock(pfn);
}

// Convert a physblock pointer to its associated PFN.
static inline pfn_t physblock_to_pfn(struct physblock *block)
{
	uint64_t bytes = (uint64_t)block - KERNEL_MEM_MAP_ADDRESS;
	pfn_t pfn = {bytes / sizeof(struct physblock)};
	return pfn;
}

// Covert a physblock pointer to its associated physical address.
static inline physaddr_t physblock_to_phys(struct physblock *block)
{
	pfn_t pfn = physblock_to_pfn(block);
	return pfn_to_phys(pfn);
}

// Determine the PFN of the 'buddy' page to the page specified by `pfn`.
static inline pfn_t pfn_to_buddy_pfn(pfn_t pfn, uint8_t order)
{
	// We simply use the lowest bit in the PFN to indicate which of the pair
	// this page is, so to find the other we simply xor it.
	// E.g. 0b10100 of order 1 would pair with 0b10110 and vice-versa.
	pfn_t ret = {pfn.x ^ (1UL << order)};
	return ret;
}

// Initialise the physical allocator _state_, `ptr` points to an early allocated
// memory page. Called during early memory initialisation.
struct early_page_alloc_state;
void phys_alloc_init_state(void *ptr, struct early_page_alloc_state *early_state);

// Gets the physical allocator state, with a lock acquired.
struct phys_alloc_state *phys_get_alloc_state_lock(void);

// Decrements reference count for specified physical page, if it reaches zero
// the page is freed.
void phys_free_pfn(pfn_t pfn);

// Decrements reference count for specified physical page, if it reaches zero
// the page is freed.
static inline void phys_free(physaddr_t pa)
{
	phys_free_pfn(phys_to_pfn(pa));
}

// Actually initialise the full-fat physical memory allocator.
void phys_alloc_init(void);

// Determine PFN span index, or -1 if not contained within known memory.
// ASSUMES: `alloc_state` has lock held.
int pfn_to_span_locked(pfn_t pfn);

// Allocate physically contiguous memory consisting of 2^order 4 KiB pages and
// returns the physblock associated with this memory.
struct physblock *phys_alloc_block(uint8_t order, alloc_flags_t flags);

// Allocate physically contiguous memory consisting of 2^order 4 KiB pages and
// returns the physical address associated with this memory.
static inline physaddr_t phys_alloc(uint8_t order, alloc_flags_t flags)
{
	struct physblock *block = phys_alloc_block(order, flags);
	return physblock_to_phys(block);
}

// Allocate a single 4 KiB page of kernel allocation type.
static inline physaddr_t phys_alloc_one(void)
{
	return phys_alloc(0, ALLOC_KERNEL);
}
