#pragma once

#include "atomic.h"
#include "spinlock.h"
#include "types.h"

// Represents the state of the physical memory block. Multiple flags can be
// assigned at the same time.
enum phys_flag {
	PHYS_ALLOCATED_BIT = 0,
	PHYS_PINNED_BIT = 1,
	PHYS_UNMOVEABLE_BIT = 2,
};

// Represents a block of physical memory which is of size 2^order pages. We
// store an array of these at a set position in memory for quick access,
// describing all available physical memory
struct phys_block {
	int16_t head_offset; // Offset to phys_block that describes this block
			     // of memory.
	uint8_t order;	     // This physical block is of size 2^order pages.
	uint8_t flags;	     // Flags describing this physical block.

	uint32_t pad;

	atomic_t refcount; // Reference count.
	spinlock_t lock;   // Lock.
};
static_assert(sizeof(struct phys_block) == 2 * sizeof(uint64_t));

// Used to locate the head of a block of physical memory from any page within
// its range.
static inline struct phys_block *phys_head(struct phys_block *block)
{
	// We take the risk of not taking a lock on a tail page.
	return block - block->head_offset;
}

// Obtain a copy of a specific physical block.
static inline struct phys_block phys_read(struct phys_block *block)
{
	struct phys_block *ptr = phys_head(block);

	spinlock_acquire(&ptr->lock);
	struct phys_block ret = *ptr;
	spinlock_release(&ptr->lock);

	return ret;
}
