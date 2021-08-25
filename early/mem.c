#include "zeptux_early.h"

// Stores scratch allocator state. We will only access this single-threaded so
// it's ok to keep this static.
static struct scratch_alloc_state scratch_state;

// Drop the direct mapping from VA 0 / PA 0. We don't need it any more.
static void drop_direct0(void)
{
	memset((void *)EARLY_PUD_DIRECT0_ADDRESS, 0, PAGE_SIZE);
	memset((void *)EARLY_PGD_ADDRESS, 0, 8);
	global_flush_tlb();
}

// Compare 2 e820 entries and determine which one is 'less' than the other. If
// the bases are equal we tie-break on size.
static bool less(struct e820_entry *a, struct e820_entry *b)
{
	return a->base < b->base || (a->base == b->base && a->size < b->size);
}

// Initialise the early scratch allocator, using pages placed immediately after
// the ELF. These pages will _not_ be kept later.
static void early_scratch_alloc_init(struct early_boot_info *info)
{
	// We place the scratch buffer immediately after the ELF...
	uint64_t offset = KERNEL_ELF_ADDRESS_PHYS + info->kernel_elf_size_bytes;
	// ...page aligned.
	physaddr_t addr = {ALIGN_UP(offset, PAGE_SIZE)};
	scratch_state.start = addr;
	scratch_state.pages = 0;
}

// Delete an entry at `index` in the e820 array. This is O(n).
static void delete_e820_entry(struct early_boot_info *info, int index)
{
	// This should never happen but for the sake of avoiding underflow,
	// check for it.
	if (info->num_e820_entries == 0)
		early_panic("0 e820 entries?");

	for (int i = index; i < (int)info->num_e820_entries - 1; i++) {
		info->e820_entries[i] = info->e820_entries[i + 1];
	}
	info->num_e820_entries--;
}

void early_sort_e820(struct early_boot_info *info)
{
	// Insertion sort both because n is small and it's highly likely this
	// will already be sorted which would make this O(n).
	for (int i = 1; i < (int)info->num_e820_entries; i++) {
		struct e820_entry key = info->e820_entries[i];

		int j;
		for (j = i - 1; j >= 0 && !less(&info->e820_entries[j], &key);
		     j--) {
			info->e820_entries[j + 1] = info->e820_entries[j];
		}
		info->e820_entries[j + 1] = key;
	}
}

void early_merge_e820(struct early_boot_info *info)
{
	// Note that we assume that all zero-size entries have been pruned out
	// by the boot code AND that the entries have been sorted.

	int curr_index = 0;
	for (int i = 1; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *curr = &info->e820_entries[curr_index];
		struct e820_entry *entry = &info->e820_entries[i];
		range_pair_t curr_range = {
			.from = curr->base,
			.to = curr->base + curr->size - 1,
		};
		range_pair_t entry_range = {
			.from = entry->base,
			.to = entry->base + entry->size - 1,
		};

		if (entry->type != curr->type)
			goto keep;

		// We've sorted the arrays so we know which order the ranges are
		// in.
		switch (range_compare(curr_range, entry_range)) {
		case RANGE_SEPARATE:
			// ....
			//      ~ ####
			break;
		case RANGE_ADJACENT:
		case RANGE_OVERLAP:
			// ....        ....
			//     #### or   ####
			curr->size += entry_range.to - curr_range.to;
			goto loop;
		case RANGE_COINCIDE:
			// ......
			//  ####
			goto loop;
		}

	keep:
		curr_index++;
		if (curr_index != i)
			info->e820_entries[curr_index] = *entry;

	loop:
	}

	info->num_e820_entries = curr_index + 1;
}

void early_normalise_e820(struct early_boot_info *info)
{
	// Since we can only map page-aligned physical addresses those parts of
	// available physical memory that start or end unaligned are simply not
	// safely available to us. To ensure we only access available memory,
	// page align all RAM entries.
	for (int i = 0; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *entry = &info->e820_entries[i];

		if (entry->type != E820_TYPE_RAM)
			continue;

		uint64_t start = entry->base;
		uint64_t end = entry->base + entry->size; // Exclusive.

		// Most likely situation - we are already page-aligned and so
		// have nothing to do.
		if (IS_ALIGNED(start, PAGE_SIZE) && IS_ALIGNED(end, PAGE_SIZE))
			continue;

		// Align start to page size, rounding up so we don't access
		// unavailable memory.
		start = ALIGN_UP(start, PAGE_SIZE);
		// Align end to page size, rounding down so we don't access
		// unavailable memory.
		end = ALIGN(end, PAGE_SIZE);

		// We had to page-align, but the entry wasn't so tiny that it
		// meant a page-aligned page of data wasn't available.
		if (start < end) {
			entry->base = start;
			entry->size = end - start;
			continue;
		}

		// This is rather unlikely but conceivable - the entry is so
		// small that it doesn't have a page-aligned page of data
		// available, so we have to delete it.

		// If we had to do this for enough entries this would be
		// O(n^2). However this scenario is unlikely and n is small in
		// any case.
		delete_e820_entry(info, i);
		i--;
	}
}

uint64_t early_get_total_ram(struct early_boot_info *info)
{
	uint64_t ret = 0;

	for (int i = 0; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *entry = &info->e820_entries[i];

		if (entry->type == E820_TYPE_RAM)
			ret += entry->size;
	}

	return ret;
}

void early_mem_init(void)
{
	struct early_boot_info *info = early_get_boot_info();

	drop_direct0();
	early_sort_e820(info);
	early_merge_e820(info);
	early_normalise_e820(info);
	info->total_avail_ram_bytes = early_get_total_ram(info);
	early_scratch_alloc_init(info);
}

struct scratch_alloc_state *early_scratch_alloc_state(void)
{
	return &scratch_state;
}

physaddr_t early_scratch_page_alloc(void)
{
	// About as simplistic as it gets.
	physaddr_t addr = {scratch_state.start.x +
			   PAGE_SIZE * scratch_state.pages++};

	// Zero the page.
	uint8_t *ptr = phys_to_virt_ptr(addr);
	memset(ptr, 0, PAGE_SIZE);

	return addr;
}
