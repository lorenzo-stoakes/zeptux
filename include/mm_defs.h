#pragma once

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
// Where we place vmalloc mappings.
#define KERNEL_VMALLOC_ADDRESS (X86_KERNEL_VMALLOC_ADDRESS)
// The physical address of the kernel stack.
#define KERNEL_STACK_ADDRESS_PHYS (X86_KERNEL_STACK_ADDRESS_PHYS)
// The maximum number of pages available in the kernel stack.
#define KERNEL_STACK_PAGES (4)
// The virtually mapped APIC base address.
#define APIC_BASE_ADDRESS (0xffffc10040000000UL)

// The maximum 2^order size of a physically contiguous allocation.
#define MAX_ORDER (12)
