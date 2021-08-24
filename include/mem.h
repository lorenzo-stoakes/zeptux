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
#define KERNEL_ELF_ADDRESS_PHYS (X86_KERNEL_ELF_OFFSET)
#define KERNEL_ELF_ADDRESS (X86_KERNEL_ELF_ADDRESS)
// Where the .text section of the kernel is loaded to.
#define KERNEL_TEXT_ADDRESS (X86_KERNEL_TEXT_ADDRESS)

// Set the `dest` buffer to contain `count` bytes of `chr`.
static inline void *memset(void *dest, int chr, uint64_t count)
{
	// A naive implementation, something to get us started.

	char *target = dest;

	for (uint64_t i = 0; i < count; i++) {
		*target++ = chr;
	}

	return dest;
}

// Copy memory from `src` to `dest` of size `n` bytes. Returns `dest` for
// convenience.
static inline void *memcpy(void *dest, void *src, size_t n)
{
	// A naive implementation, something to get us started.

	uint8_t *ptr = dest;

	for (size_t i = 0; i < n; i++) {
		*ptr++ = *(uint8_t *)src++;
	}

	return dest;
}
