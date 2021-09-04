#include "test_early.h"

const char *test_misc(void)
{
	assert(ALIGN(1024, 2) == 1024, "ALIGN(1024, 2) != 1024");
	assert(ALIGN(1025, 2) == 1024, "ALIGN(1025, 2) != 1024");
	assert(ALIGN(0, 2) == 0, "ALIGN(0, 2) != 0");
	assert(ALIGN(0, 512) == 0, "ALIGN(0, 512) != 0");

	assert(ALIGN_UP(0, 2) == 0, "ALIGN_UP(0, 2) != 2");
	assert(ALIGN_UP(0, 512) == 0, "ALIGN_UP(0, 512) != 512");
	assert(ALIGN_UP(1025, 2) == 1026, "ALIGN_UP(1025, 2) != 1026");

	const uint16_t shorts[] = {1, 2, 3};
	assert(ARRAY_COUNT(shorts) == 3, "ARRAY_COUNT({1, 2, 3}) != 3");

	const uint64_t empty[] = {};
	assert(ARRAY_COUNT(empty) == 0, "ARRAY_COUNT({}) != 0");

	assert(IS_ALIGNED(24, 8), "!IS_ALIGNED(24, 8)?");
	assert(!IS_ALIGNED(25, 8), "IS_ALIGNED(25, 8)?");
	assert(IS_ALIGNED(0x5000, 0x1000), "!IS_ALIGNED(0x5000, 0x1000)?");
	assert(!IS_ALIGNED(0x5fff, 0x1000), "IS_ALIGNED(0x5fff, 0x1000)?");

	physaddr_t pa = {0x1fff};
	pa = pa_next_page(pa);
	assert(pa.x == 0x2000,
	       "pa_next_page() not moving correctly to next page");
	pa = pa_next_page(pa);
	assert(pa.x == 0x3000,
	       "pa_next_page() not moving correctly to next page");
	pa = pa_prev_page(pa);
	assert(pa.x == 0x2000,
	       "pa_next_page() not moving correctly to previous page");

	assert(bytes_to_pages(0) == 0, "bytes_to_pages(0) != 0?");
	assert(bytes_to_pages(0x1000) == 1, "bytes_to_pages(0x1000) != 1?");
	assert(bytes_to_pages(0x1001) == 2, "bytes_to_pages(0x1001) != 2?");

	pa.x = 0xdeadbeef;
	assert(pa_to_pfn(pa).x == 0xdeadb, "0xdeadbeef PFN != 0xdeadb?");

	return NULL;
}
