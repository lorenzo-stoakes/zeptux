#pragma once

#include "atomic.h"
#include "spinlock.h"
#include "types.h"

enum phys_flag {
	PHYS_PINNED_BIT = 0,
	PHYS_UNMOVEABLE_BIT = 1,
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
	return block - block->head_offset;
}

// Is this physical block of memory pinned (temporarily unmovable/unfreeable)?
static inline bool phys_is_pinned(struct phys_block *block)
{
	block = phys_head(block);
	return IS_SET(block->flags, PHYS_PINNED_BIT);
}

// Is this physical block of memory movable?
static inline bool phys_is_movable(struct phys_block *block)
{
	block = phys_head(block);
	return !IS_SET(block->flags, PHYS_PINNED_BIT);
}
