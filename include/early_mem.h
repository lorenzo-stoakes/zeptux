#pragma once

#include "bitmap.h"
#include "page.h"
#include "types.h"

// Initial kernel stack location.
#define EARLY_KERNEL_INIT_STACK (X86_KERNEL_INIT_STACK)
// Location where we store intial boot information useful for the kernel.
#define EARLY_BOOT_INFO_ADDRESS (X86_EARLY_BOOT_INFO_ADDRESS)
// Location in memory where text output can be written to which gets displayed
// on the monitor.
#define EARLY_VIDEO_MEMORY_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_VIDEO_MEMORY_ADDRESS_PHYS)
// Location in memory which acts as a double buffer to the actual video memory.
#define EARLY_VIDEO_BUFFER_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_VIDEO_BUFFER_ADDRESS_PHYS)

// Early page table addresses.
#define EARLY_PGD_ADDRESS (KERNEL_DIRECT_MAP_BASE + X86_EARLY_PGD)
#define EARLY_PUD_DIRECT0_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_PUD_DIRECT0)
#define EARLY_PUD_DIRECT_MAP_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_PUD_DIRECT_MAP)
#define EARLY_PUD_KERNEL_ELF (KERNEL_DIRECT_MAP_BASE + X86_EARLY_PUD_KERNEL_ELF)

struct early_boot_info; // To avoid circular declaration.

// The scratch allocator is our super-early allocator. Nothing from this
// allocated is kept when we move to our full-fat buddy allocator and its
// primary purpose is to hold state for our early allocator.
//
// We simply allocate pages from the page-aligned end of the ELF image in memory
// as nothing else will be using this memory at this stage.
struct scratch_alloc_state {
	physaddr_t start;
	uint64_t pages;
};

// Represents a span of page-aligned contiguous physical memory managed by the
// early page allocator.
struct early_page_alloc_span {
	physaddr_t start;
	uint64_t num_pages;
	struct bitmap *alloc_bitmap;
	struct bitmap *ephemeral_bitmap;
};

// Represents the state of the early page allocator. All non-ephemeral allocated
// pages will be preserved when switching to the 'real' physical allocator.
struct early_page_alloc_state {
	uint64_t total_pages;
	uint64_t allocated_pages; // Includes ephemeral pages.
	uint64_t ephemeral_pages;
	uint64_t num_spans;
	struct early_page_alloc_span spans[];
};
// We place the early page allocator state and span structs in a single memory
// page allocated by the scratch allocator. This puts a natural limit on how
// many spans we can have (which maps one-to-one to the number of e820 RAM
// entries) equal to the page size less the size of the allocator state.
#define MAX_E820_RAM_ENTRIES                                   \
	((PAGE_SIZE - sizeof(struct early_page_alloc_state)) / \
	 sizeof(struct early_page_alloc_span))

// Sort e820 entries inline.
void early_sort_e820(struct early_boot_info *info);
// Merge overlapping, coincidental and adjacent e820 blocks of equal type.
// IMPORTANT: Assumes e820 entries have been sorted.
void early_merge_e820(struct early_boot_info *info);
// Normalise e820 RAM entries to page boundaries.
// IMPORTANT: Assumes e820 entries have been merged.
void early_normalise_e820(struct early_boot_info *info);
// Extract the total available memory in bytes.
// IMPORTANT: Assumes e820 entries have been merged.
uint64_t early_get_total_ram(struct early_boot_info *info);

// Retrieve scratch allocator state.
struct scratch_alloc_state *early_scratch_alloc_state(void);
// Allocate single, zeroed, page from scratch allocator.
physaddr_t early_scratch_page_alloc(void);

// Performs early memory intialisation.
void early_mem_init(void);
