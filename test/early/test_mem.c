#include "test_early.h"

#define MAX_NUM_E820_ENTRIES (100)
#define BUF_SIZE (10 + 20 * MAX_NUM_E820_ENTRIES)

static const char *assert_e820_sorted(struct early_boot_info *info)
{
	// Assert that we are sorting on size too.
	info->num_e820_entries = 4;

	info->e820_entries[0].base = 1000;
	info->e820_entries[0].size = 100;

	info->e820_entries[1].base = 500;
	info->e820_entries[1].size = 50;

	info->e820_entries[2].base = 500;
	info->e820_entries[2].size = 10;

	info->e820_entries[3].base = 501;
	info->e820_entries[3].size = 10;

	sort_e820(info);

	assert(info->e820_entries[0].base == 500, "unsorted");
	assert(info->e820_entries[0].size == 10, "unsorted");
	assert(info->e820_entries[1].base == 500, "unsorted");
	assert(info->e820_entries[1].size == 50, "unsorted");
	assert(info->e820_entries[2].base == 501, "unsorted");
	assert(info->e820_entries[2].size == 10, "unsorted");
	assert(info->e820_entries[3].base == 1000, "unsorted");
	assert(info->e820_entries[3].size == 100, "unsorted");

	// Now simply check for a large standard sort.
	info->num_e820_entries = MAX_NUM_E820_ENTRIES;
	for (int i = 0; i < MAX_NUM_E820_ENTRIES; i++) {
		info->e820_entries[i].base = MAX_NUM_E820_ENTRIES - i;
		info->e820_entries[i].size = 10;
	}

	sort_e820(info);

	for (int i = 0; i < MAX_NUM_E820_ENTRIES; i++) {
		if (info->e820_entries[i].base != (uint64_t)i + 1) {
			early_printf(
				"assert_e820_sorted: [%d].base=%lu, expected %d\n",
				i, info->e820_entries[i].base, i + 1);

			assert(false, "unsorted");
		}
	}

	return NULL;
}

static void setup_merge_entries(struct early_boot_info *info)
{
	// The input (inclusive ranges):
	// 0 - 9; [separate] 11 - 20; [overlap] 15 - 21;
	// [separate] 30 - 50; [coincide] 30-35; [coincide] 40-45;
	// [overlap] 50 - 51; [separate] 55 - 58; [adjacent] 59 - 63;
	// [adjacent] 64 - 66

	// We expect this to be merged to:

	// 0 - 9; 11 - 21; 30 - 51; 55 - 66;

	info->num_e820_entries = 10;

	info->e820_entries[0].base = 0;
	info->e820_entries[0].size = 10;

	// Separate from 1ast.
	info->e820_entries[1].base = 11;
	info->e820_entries[1].size = 10;

	// Overlaps with last.
	info->e820_entries[2].base = 15;
	info->e820_entries[2].size = 7;

	// Separate from last.
	info->e820_entries[3].base = 30;
	info->e820_entries[3].size = 21;

	// Coincides with last.
	info->e820_entries[4].base = 30;
	info->e820_entries[4].size = 6;

	// Coincides with 3.
	info->e820_entries[5].base = 40;
	info->e820_entries[5].size = 6;

	// Overlaps 3.
	info->e820_entries[6].base = 50;
	info->e820_entries[6].size = 2;

	// Separate from last.
	info->e820_entries[7].base = 55;
	info->e820_entries[7].size = 4;

	// Adjacent to last.
	info->e820_entries[8].base = 59;
	info->e820_entries[8].size = 5;

	// Adjacent to last.
	info->e820_entries[9].base = 64;
	info->e820_entries[9].size = 3;
}

static const char *assert_e820_merged(struct early_boot_info *info)
{
	// As memory is zeroed, all will have the same type == 0.

	setup_merge_entries(info);
	merge_e820(info);

	// We should end up with:
	// 0 - 9; 11 - 21; 30 - 51; 55 - 66;

	assert(info->num_e820_entries == 4, "merged e820 entries != 4");
	assert(info->e820_entries[0].base == 0, "[0] base != 0");
	assert(info->e820_entries[0].size == 10, "[0] size != 10");

	assert(info->e820_entries[1].base == 11, "[1] base != 11");
	assert(info->e820_entries[1].size == 11, "[1] size != 11");

	assert(info->e820_entries[2].base == 30, "[2] base != 30");
	assert(info->e820_entries[2].size == 22, "[2] size != 22");

	assert(info->e820_entries[3].base == 55, "[3] base != 55");
	assert(info->e820_entries[3].size == 12, "[3] size != 12");

	// Now try with different types:

	// Make the first adjancent entry of different type. This is really the
	// only plausible way in which merges will be impacted by different
	// memory types as they could never overlap (if they did, the BIOS is
	// buggy!)
	setup_merge_entries(info);
	info->e820_entries[8].type = E820_TYPE_RESERVED;

	merge_e820(info);

	// We should end up with:
	// 0 - 9; 11 - 21; 30 - 51; 55 - 58; [RAM] 59 - 63; [0] 64 - 66

	assert(info->num_e820_entries == 6, "merged e820 entries != 6");

	assert(info->e820_entries[0].base == 0, "[0] base != 0");
	assert(info->e820_entries[0].size == 10, "[0] size != 10");

	assert(info->e820_entries[1].base == 11, "[1] base != 11");
	assert(info->e820_entries[1].size == 11, "[1] size != 11");

	assert(info->e820_entries[2].base == 30, "[2] base != 30");
	assert(info->e820_entries[2].size == 22, "[2] size != 22");

	assert(info->e820_entries[3].base == 55, "[3] base != 55");
	assert(info->e820_entries[3].size == 4, "[3] size != 4");
	assert(info->e820_entries[3].type == 0, "[3] type != 0");

	assert(info->e820_entries[4].base == 59, "[4] base != 59");
	assert(info->e820_entries[4].size == 5, "[4] size != 5");
	assert(info->e820_entries[4].type == E820_TYPE_RESERVED,
	       "[4] type != reserved");

	assert(info->e820_entries[5].base == 64, "[5] base != 64");
	assert(info->e820_entries[5].size == 3, "[5] size != 3");
	assert(info->e820_entries[5].type == 0, "[5] type != 0");

	return NULL;
}

static const char *assert_correct_total_ram(struct early_boot_info *info)
{
	info->num_e820_entries = 4;

	// This assumes e820 entries are merged so only thing to test are that
	// we correct sum RAM entries only.

	info->e820_entries[0].base = 0;
	info->e820_entries[0].size = 100;
	info->e820_entries[0].type = E820_TYPE_RAM;

	info->e820_entries[1].base = 1000;
	info->e820_entries[1].size = 12345;
	info->e820_entries[1].type = E820_TYPE_RESERVED;

	info->e820_entries[2].base = 2000;
	info->e820_entries[2].size = 999;
	info->e820_entries[2].type = E820_TYPE_RAM;

	info->e820_entries[3].base = 50000;
	info->e820_entries[3].size = 1;
	info->e820_entries[3].type = E820_TYPE_RAM;

	assert(get_total_ram(info) == 1100, "total ram != 1100");

	return NULL;
}

const char *test_mem(void)
{
	uint8_t buf[BUF_SIZE] = {0};
	struct early_boot_info *info = (struct early_boot_info *)buf;

	const char *ret = assert_e820_sorted(info);
	if (ret != NULL)
		return ret;

	memset(buf, 0, BUF_SIZE);
	ret = assert_e820_merged(info);
	if (ret != NULL)
		return ret;

	memset(buf, 0, BUF_SIZE);
	ret = assert_correct_total_ram(info);

	return ret;
}
