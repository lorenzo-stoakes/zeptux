#include "test.h"
#include "zeptux_early.h"

const char *test_misc(void)
{
	assert(ALIGN(1024, 2) == 1024, "ALIGN(1024, 2) != 1024");
	assert(ALIGN(1025, 2) == 1024, "ALIGN(1025, 2) != 1024");
	assert(ALIGN(0, 2) == 0, "ALIGN(0, 2) != 0");
	assert(ALIGN(0, 512) == 0, "ALIGN(0, 512) != 0");

	assert(ALIGN_UP(0, 2) == 0, "ALIGN_UP(0, 2) != 2");
	assert(ALIGN_UP(0, 512) == 0, "ALIGN_UP(0, 512) != 512");
	assert(ALIGN_UP(1025, 2) == 1026, "ALIGN_UP(1025, 2) != 1026");

	const uint16_t shorts[] = { 1, 2, 3 };
	assert(ARRAY_COUNT(shorts) == 3, "ARRAY_COUNT({1, 2, 3}) != 3");

	const uint64_t empty[] = {};
	assert(ARRAY_COUNT(empty) == 0, "ARRAY_COUNT({}) != 0");

	return NULL;
}