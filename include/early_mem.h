#pragma once

#include "bitmap.h"
#include "elf.h"
#include "page.h"
#include "types.h"

// Location where we store intial boot information useful for the kernel.
#define EARLY_BOOT_INFO_ADDRESS (X86_EARLY_BOOT_INFO_ADDRESS)
// Location in memory where text output can be written to which gets displayed
// on the monitor.
#define EARLY_VIDEO_MEMORY_ADDRESS_PHYS X86_VIDEO_MEMORY_ADDRESS_PHYS
#define EARLY_VIDEO_MEMORY_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + EARLY_VIDEO_MEMORY_ADDRESS_PHYS)
// Location in memory which acts as a double buffer to the actual video memory.
#define EARLY_VIDEO_BUFFER_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_VIDEO_BUFFER_ADDRESS_PHYS)

// Early page table addresses.
#define EARLY_PGD_ADDRESS (KERNEL_DIRECT_MAP_BASE + X86_EARLY_PGD)
#define EARLY_PUD_DIRECT0_ADDRESS (KERNEL_DIRECT_MAP_BASE + X86_EARLY_PUD_DIRECT0)
#define EARLY_PUD_DIRECT_MAP_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_PUD_DIRECT_MAP)
#define EARLY_PUD_KERNEL_ELF_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_PUD_KERNEL_ELF)

// See https://uefi.org/specs/ACPI/6.4/15_System_Address_Map_Interfaces/int-15h-e820h---query-system-address-map.html
struct e820_entry {
	uint64_t base;
	uint64_t size;
	uint32_t type;
} PACKED;

// Represents system information we can only obtain on boot in real mode or
// externally (in the case of kernel ELF bytes). This data structure is stored
// at a specific known location in memory which the kernel can access once
// booted.
struct early_boot_info {
	uint64_t kernel_elf_size_bytes;
	uint64_t total_avail_ram_bytes;
	uint64_t num_ram_spans;
	uint64_t num_e820_entries;
	struct e820_entry e820_entries[0];
} PACKED;

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
	uint64_t num_allocated_pages;
	struct bitmap *alloc_bitmap;
	struct bitmap *ephemeral_bitmap;
	struct bitmap *pagetable_bitmap;
};

// Represents the state of the early page allocator. All non-ephemeral allocated
// pages will be preserved when switching to the 'real' physical allocator.
struct early_page_alloc_state {
	uint64_t total_pages;
	uint64_t num_allocated_pages; // Includes ephemeral pages.
	uint64_t num_ephemeral_pages;
	uint64_t num_pagetable_pages;
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

// See https://uefi.org/specs/ACPI/6.4/15_System_Address_Map_Interfaces/Sys_Address_Map_Interfaces.html#address-range-types
enum e820_type {
	E820_TYPE_RAM = 1,
	E820_TYPE_RESERVED = 2,
	E820_TYPE_ACPI = 3,
	E820_TYPE_NVS = 4,
	E820_TYPE_UNUSABLE = 5,
	E820_TYPE_DISABLED = 6,
	E820_TYPE_PERSISTENT = 7,
};

// Obtains the early boot information structure via the direct mapping.
static inline struct early_boot_info *early_get_boot_info(void)
{
	return (struct early_boot_info *)EARLY_BOOT_INFO_ADDRESS;
}

// Convert an e820 entry type to string for debug output.
static inline const char *e820_type_to_string(enum e820_type type)
{
	switch (type) {
	case E820_TYPE_RAM:
		return "RAM";
	case E820_TYPE_RESERVED:
		return "reserved";
	case E820_TYPE_ACPI:
		return "ACPI";
	case E820_TYPE_NVS:
		return "NVS";
	case E820_TYPE_UNUSABLE:
		return "unusable";
	case E820_TYPE_DISABLED:
		return "disabled";
	case E820_TYPE_PERSISTENT:
		return "persistent";
	default:
		return "UNKNOWN";
	}
}

// Sort e820 entries inline.
void early_sort_e820(struct early_boot_info *info);
// Merge overlapping, coincidental and adjacent e820 blocks of equal type.
// IMPORTANT: Assumes e820 entries have been sorted.
void early_merge_e820(struct early_boot_info *info);
// Normalise e820 RAM entries to page boundaries.
// IMPORTANT: Assumes e820 entries have been merged.
void early_normalise_e820(struct early_boot_info *info);
// Extract the total available memory in bytes and set in info object.
// IMPORTANT: Assumes e820 entries have been merged.
void early_set_total_ram(struct early_boot_info *info);

// Retrieve scratch allocator state.
struct scratch_alloc_state *early_scratch_alloc_state(void);
// Allocate single, zeroed, page from scratch allocator.
physaddr_t early_scratch_page_alloc(void);

// Performs early memory intialisation.
void early_mem_init(void);

// Initialise the early page allocator.
void early_page_alloc_init(struct early_boot_info *info);

// Allocate ephemeral page at specific physical address. Panics if already
// allocated. It will NOT be zeroed.
void early_page_alloc_ephemeral_at(physaddr_t pa);

// Allocate page at specific physical address. Panics if already allocated. It
// will NOT be zeroed.
void early_page_alloc_at(physaddr_t pa);

// Allocates physical page from the early page allocator. It will NOT be zeroed.
physaddr_t early_page_alloc(void);

// Allocates physical page from the early page allocator and zeroes it.
static inline physaddr_t early_page_alloc_zero(void)
{
	physaddr_t pa = early_page_alloc();

	zero_page(pa);
	return pa;
}

// Allocate a physical page intended to be used for a page table. It WILL be
// zeroed.
physaddr_t early_pagetable_alloc(void);

// Allocates an ephemeral physical page from the early page allocator (will be
// discared when switching to the full fat physical allocator). It will NOT be
// zeroed.
physaddr_t early_page_alloc_ephemeral(void);

// Free a page from the early page allocator. If the page is not currently
// allocated, this function will panic (early page allocation/freeing should not
// be done casually!)
void early_page_free(physaddr_t pa);

// Maps physical memory as specified in RAM entries in the boot info to the
// specified PGD.
void early_map_direct(struct early_boot_info *info, pgdaddr_t pgd);

// Retrieve early page alloc state. Mostly exposed for testability.
struct early_page_alloc_state *early_get_page_alloc_state(void);

// Move from the early page table structure (1 GiB direct, ELF image mpaping) to
// an actually correct mapping.
void early_remap_page_tables(struct early_boot_info *info);

// Map kernel ELF into memory using the specific ELF header (it is assumed the
// pointer points to beginning of the ELF) mapping num_pages into memory and
// being careful to map readonly sections as readonly, data sections as NX and
// executable sections as non-NX.
void early_map_kernel_elf(struct elf_header *header, physaddr_t pa,
			  pgdaddr_t pgd);

// Initialise an array of physblocks describing physical memory, allocating
// pages to store this information and then mapping from
// KERNEL_MEM_MAP_ADDRESS. We index into this array by PFN.
void early_init_mem_map(void);

// Generate early page allocation functions for each page level.
#define GEN_PAGE_ALLOC(pagelevel)                                     \
	static inline pagelevel##addr_t early_alloc_##pagelevel(void) \
	{                                                             \
		physaddr_t pa = early_pagetable_alloc();              \
		pagelevel##addr_t ret = {pa.x};                       \
		return ret;                                           \
	}
GEN_PAGE_ALLOC(pgd);
GEN_PAGE_ALLOC(pud);
GEN_PAGE_ALLOC(pmd);
GEN_PAGE_ALLOC(ptd);
#undef GEN_PAGE_ALLOC

// Generate early page free functions for each page level.
#define GEN_PAGE_FREE(pagelevel)                                          \
	static inline void early_free_##pagelevel(pagelevel##addr_t addr) \
	{                                                                 \
		physaddr_t pa = {addr.x};                                 \
		early_page_free(pa);                                      \
	}
GEN_PAGE_FREE(pgd);
GEN_PAGE_FREE(pud);
GEN_PAGE_FREE(pmd);
GEN_PAGE_FREE(ptd);
#undef GEN_PAGE_FREE
