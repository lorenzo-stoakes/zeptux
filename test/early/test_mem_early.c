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

	early_sort_e820(info);

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

	early_sort_e820(info);

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
	early_merge_e820(info);

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

	early_merge_e820(info);

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

static const char *assert_e820_normalised(struct early_boot_info *info)
{
	info->num_e820_entries = 6;

	// Too small and must be deleted.
	info->e820_entries[0].base = 1;
	info->e820_entries[0].size = 0x1000;
	info->e820_entries[0].type = E820_TYPE_RAM;

	// Only just big enough for both start and end to need to be adjusted.
	info->e820_entries[1].base = 0x1001;
	info->e820_entries[1].size = 0x3001;
	info->e820_entries[1].type = E820_TYPE_RAM;

	// Too small and must be deleted.
	info->e820_entries[2].base = 0x5000;
	info->e820_entries[2].size = 0x999;
	info->e820_entries[2].type = E820_TYPE_RAM;

	// Start fine, end needs adjustment.
	info->e820_entries[3].base = 0x10000;
	info->e820_entries[3].size = 0x1001;
	info->e820_entries[3].type = E820_TYPE_RAM;

	// Totally fine.
	info->e820_entries[4].base = 0x20000;
	info->e820_entries[4].size = 0x3000;
	info->e820_entries[4].type = E820_TYPE_RAM;

	// Too small and must be deleted.
	info->e820_entries[5].base = 0xffffffff00000001UL;
	info->e820_entries[5].size = 0x1ffe;
	info->e820_entries[5].type = E820_TYPE_RAM;

	early_normalise_e820(info);

	assert(info->num_e820_entries == 3, "Empty entries still present?");

	// Was at index 1.
	assert(info->e820_entries[0].base == 0x2000, "[0].base != 0x2000?");
	assert(info->e820_entries[0].size == 0x2000, "[0].size != 0x2000?");
	// Was at index 3.
	assert(info->e820_entries[1].base == 0x10000, "[1].base != 0x10000?");
	assert(info->e820_entries[1].size == 0x1000, "[1].size != 0x1000?");
	// Was at index 4.
	assert(info->e820_entries[2].base == 0x20000, "[2].base != 0x20000?");
	assert(info->e820_entries[2].size == 0x3000, "[2].size != 0x3000?");

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

	assert(early_get_total_ram(info) == 1100, "total ram != 1100");
	assert(info->num_ram_spans == 3, "RAM spans not set to 3?");

	return NULL;
}

const char *assert_correct_virtaddr(void)
{
	uint64_t offset = 0;

	// PGD index.
	offset |= 0xde;
	offset <<= 9;
	// PUD index.
	offset |= 0xad;
	offset <<= 9;
	// PMD index.
	offset |= 0xbe;
	offset <<= 9;
	// PTD index.
	offset |= 0xef;
	offset <<= 12;
	// Data page offset.
	offset |= 1234;

	virtaddr_t va = {offset};

	assert(virt_data_offset(va) == 1234, "data offset != 1234");
	assert(virt_ptde_index(va) == 0xef, "ptde index != 0xef");
	assert(virt_pmde_index(va) == 0xbe, "pmde index != 0xbe");
	assert(virt_pude_index(va) == 0xad, "pude index != 0xad");
	assert(virt_pgde_index(va) == 0xde, "pgde index != 0xde");

	return NULL;
}

const char *assert_correct_scratch_alloc(void)
{
	struct scratch_alloc_state *state = early_scratch_alloc_state();
	struct early_boot_info *info = early_get_boot_info();

	uint64_t orig_pages = state->pages;

	uint64_t offset = state->start.x - KERNEL_ELF_ADDRESS_PHYS;
	assert(offset == ALIGN_UP(info->kernel_elf_size_bytes, 0x1000),
	       "offset from kernel ELF PA != page-aligned kernel ELF size");

	for (int i = 0; i < 10; i++) {
		physaddr_t pa = early_scratch_page_alloc();
		assert(state->pages == orig_pages + (uint64_t)(i + 1),
		       "Scratch page count incorrect");

		uint8_t *ptr = phys_to_virt_ptr(pa);
		for (int j = 0; j < 0x1000; j++) {
			if (ptr[j] != 0) {
				early_printf(
					"test_mem: non-zeroed page at %d (alloc %d)\n",
					j, i);
				assert(false, "non-zeroed scratch page");
			}
		}
	}

	return NULL;
}

const char *assert_early_page_alloc_correct(void)
{
	// Fundamentally the bitmap checks should catch a lot of the potential
	// bugs in the early page allocator so we will take a fairly cursory
	// look at it.

	struct early_page_alloc_state *state = early_get_page_alloc_state();
	struct early_boot_info *info = early_get_boot_info();

	assert(state->total_pages > 0,
	       "early page alloc state total pages zero?");

	assert(state->num_spans == info->num_ram_spans,
	       "RAM span count mismatch between early boot info and early allocator");

	// Find first span with free memory.
	struct early_page_alloc_span *span = NULL;
	int span_index = 0;
	for (int i = 0; i < (int)state->num_spans; i++) {
		span = &state->spans[i];
		if (span->allocated_pages < span->num_pages) {
			span_index = i;
			break;
		}
	}
	// We'll allow the code to segfault in the bizarre (or broken!) scenario
	// where there is none... But check to see that there is another span
	// after us so we can test moving to it.
	assert(span_index < (int)state->num_spans,
	       "Insufficient spans to test");

	uint64_t prev_total_alloc = state->allocated_pages;
	uint64_t prev_span_alloc = span->allocated_pages;

	// Allocate the rest of the span.
	physaddr_t first_pa;
	uint64_t num_free = span->num_pages - span->allocated_pages;
	assert(num_free > 0, "Not enough free pages available in span to test");
	for (uint64_t i = 0; i < num_free; i++) {
		physaddr_t pa = early_page_alloc();
		if (i == 0)
			first_pa = pa;

		assert(state->allocated_pages == prev_total_alloc + 1,
		       "Alloc state total alloc count not incremented?");
		assert(span->allocated_pages == prev_span_alloc + 1,
		       "Span total alloc count not incremented?");

		prev_total_alloc = state->allocated_pages;
		prev_span_alloc = span->allocated_pages;

		assert(IS_ALIGNED(pa.x, PAGE_SIZE),
		       "Allocated page not page-aligned?");
	}

	struct early_page_alloc_span *next_span = &state->spans[span_index + 1];
	prev_span_alloc = next_span->allocated_pages;

	physaddr_t pa = early_page_alloc();
	assert(next_span->allocated_pages == prev_span_alloc + 1,
	       "Not allocated from next span?");
	assert(pa.x >= next_span->start.x, "Not allocated from next span?");

	// Now free from the first span.
	prev_total_alloc = state->allocated_pages;
	prev_span_alloc = span->allocated_pages;
	early_page_free(first_pa);
	assert(state->allocated_pages == prev_total_alloc - 1,
	       "Freeing didn't reduce allocated?");
	assert(span->allocated_pages == prev_span_alloc - 1,
	       "Freeing didn't reduce allocated?");

	// We should expect to get it right back.
	pa = early_page_alloc();
	assert(pa.x == first_pa.x, "Freeing doesn't allow us to reobtain PA?");

#define CHECK_ZEROED(addr)                                 \
	{                                                  \
		physaddr_t phys = {addr.x};                \
		uint8_t *ptr = phys_to_virt_ptr(phys);     \
		for (int i = 0; i < (int)PAGE_SIZE; i++) { \
			assert(*ptr == 0, "not zeroing?"); \
		}                                          \
	}

	pa = early_page_alloc_zero();
	CHECK_ZEROED(pa);
	pgdaddr_t pgd = early_alloc_pgd();
	CHECK_ZEROED(pgd);
	pudaddr_t pud = early_alloc_pud();
	CHECK_ZEROED(pud);
	pmdaddr_t pmd = early_alloc_pmd();
	CHECK_ZEROED(pmd);
	ptdaddr_t ptd = early_alloc_ptd();
	CHECK_ZEROED(ptd);

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
	ret = assert_e820_normalised(info);
	if (ret != NULL)
		return ret;

	memset(buf, 0, BUF_SIZE);
	ret = assert_correct_total_ram(info);
	if (ret != NULL)
		return ret;

	ret = assert_correct_virtaddr();
	if (ret != NULL)
		return ret;

	ret = assert_correct_scratch_alloc();
	if (ret != NULL)
		return ret;

	ret = assert_early_page_alloc_correct();
	if (ret != NULL)
		return ret;

	return NULL;
}
