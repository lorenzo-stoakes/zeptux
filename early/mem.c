#include "zeptux_early.h"

// Drop the direct mapping from VA 0 / PA 0. We don't need it any more.
static void drop_direct0(void)
{
	memset((void *)EARLY_PUD_DIRECT0_ADDRESS, 0, 0x1000);
	memset((void *)EARLY_PGD_ADDRESS, 0, 8);
	global_flush_tlb();
}

void early_meminit(void)
{
	drop_direct0();

	// 1. Sort, dedupe e820
	// 2. Determine avialable free memory
	// 3. Initialise physical memblock

	// Do some stuff yeah?
}
