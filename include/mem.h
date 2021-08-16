#pragma once

// See docs/memmap.md for more information on the physical and virtual memory
// maps as provided by hardware and assigned by zeptux.

#include "types.h"
// For now we assume x86-64 architecture.
#include "arch/x86_64/include/x86-consts.h"

// The division between userland and kernel memory.
#define KERNEL_BASE (X86_KERNEL_BASE)
// Taking advantage of the larger address space we map ALL physical memory from
// KERNEL_DIRECT_MAP (up to 64 TiB).
#define KERNEL_DIRECT_MAP_BASE (X86_KERNEL_DIRECT_MAP_BASE)
// Where we load the kernel.
#define KERNEL_ELF_ADDRESS (X86_KERNEL_ELF_ADDRESS)
// Where the .text section of the kernel is loaded to.
#define KERNEL_TEXT_ADDRESS (X86_KERNEL_TEXT_ADDRESS)
// Number of bits contained within default memory page size.
#define PAGE_SHIFT (X86_PAGE_SHIFT)
// The default memory page size.
#define PAGE_SIZE (1UL << PAGE_SHIFT)

static inline void *memset(void *dest, int chr, uint64_t count)
{
	char *target = dest;
	for (uint64_t i = 0; i < count; i++) {
		*target++ = chr;
	}

	return dest;
}

static inline void *memcpy(void *dest, void *src, size_t n)
{
	uint8_t *ptr = dest;

	for (size_t i = 0; i < n; i++) {
		*ptr++ = *(uint8_t *)src++;
	}

	return dest;
}
