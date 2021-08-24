#pragma once

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

// Sort e820 entries inline.
void early_sort_e820(struct early_boot_info *info);
// Merge overlapping, coincidental and adjacent e820 blocks of equal type.
// IMPORTANT: Assumes e820 entries have been sorted.
void early_merge_e820(struct early_boot_info *info);
// Extract the total available memory in bytes.
// IMPORTANT: Assumes e820 entries have been merged.
uint64_t early_get_total_ram(struct early_boot_info *info);

// Retrieve scratch allocator state.
struct scratch_alloc_state *early_scratch_alloc_state(void);
// Allocate single, zeroed, page from scratch allocator.
physaddr_t early_scratch_page_alloc(void);

// Performs early memory intialisation.
void early_mem_init(void);
