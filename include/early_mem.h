#pragma once

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

void sort_e820(struct early_boot_info *info);
// Assumes e820 entries sorted.
void merge_e820(struct early_boot_info *info);
// Assumes e820 entries merged.
uint64_t get_total_ram(struct early_boot_info *info);

// Returns total RAM in bytes.
uint64_t early_meminit(void);
