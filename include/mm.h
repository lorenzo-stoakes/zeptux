#pragma once

// See docs/memmap.md for more information on the physical and virtual memory
// maps as provided by hardware and assigned by zeptux.

#include "atomic.h"
#include "compiler.h"
#include "list.h"
#include "spinlock.h"
#include "types.h"
// For now we assume x86-64 architecture.
#include "arch/x86_64/include/x86-consts.h"

// The division between userland and kernel memory.
#define KERNEL_BASE (X86_KERNEL_BASE)
// Taking advantage of the larger address space we map ALL physical memory from
// KERNEL_DIRECT_MAP (up to 64 TiB).
#define KERNEL_DIRECT_MAP_BASE (X86_KERNEL_DIRECT_MAP_BASE)
// Where we load the kernel.
#define KERNEL_ELF_ADDRESS_PHYS (X86_KERNEL_ELF_ADDRESS_PHYS)
#define KERNEL_ELF_ADDRESS (X86_KERNEL_ELF_ADDRESS)
// Where the .text section of the kernel is loaded to.
#define KERNEL_TEXT_ADDRESS (X86_KERNEL_TEXT_ADDRESS)
// Where an array of struct physblock entries exist (discontiguously mapped by
// PFN such that offsetting to an existing physical page from here will provide
// the appropriate physblock)
#define KERNEL_MEM_MAP_ADDRESS (X86_KERNEL_MEM_MAP_ADDRESS)
// The physical address of the kernel stack.
#define KERNEL_STACK_ADDRESS_PHYS (X86_KERNEL_STACK_ADDRESS_PHYS)
// The maximum number of pages available in the kernel stack.
#define KERNEL_STACK_PAGES (4)

// The maximum 2^order size of a physically contiguous allocation.
#define MAX_ORDER (12)

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
