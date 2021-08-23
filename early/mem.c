#include "zeptux_early.h"

// Drop the direct mapping from VA 0 / PA 0. We don't need it any more.
static void drop_direct0(void)
{
	memset((void *)EARLY_PUD_DIRECT0_ADDRESS, 0, 0x1000);
	memset((void *)EARLY_PGD_ADDRESS, 0, 8);
	global_flush_tlb();
}

static bool less(struct e820_entry *a, struct e820_entry *b)
{
	return a->base < b->base || (a->base == b->base && a->size < b->size);
}

// Sort e820 entries inline.
void early_sort_e820(struct early_boot_info *info)
{
	// Insertion sort.
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

// Merge overlapping, coincidental and adjacent e820 blocks of equal type.
// IMPORTANT: Assumes e820 entries have been sorted.
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

// Extract the total available memory in bytes.
// IMPORTANT: Assumes e820 entries have been merged.
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

// Performs early memory intialisation, returns total RAM in bytes.
uint64_t early_meminit(void)
{
	struct early_boot_info *info = early_get_boot_info();

	drop_direct0();
	early_sort_e820(info);
	early_merge_e820(info);

	uint64_t total_ram_bytes = early_get_total_ram(info);

	// 3. Initialise physical memblock

	return total_ram_bytes;
}
