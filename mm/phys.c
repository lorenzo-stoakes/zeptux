#include "zeptux.h"

static struct phys_alloc_state *alloc_state;

// The root kernel PGD.
pgdaddr_t kernel_root_pgd;

void phys_alloc_init_state(void *ptr)
{
	alloc_state = (struct phys_alloc_state *)ptr;

	for (int i = 0; i < MAX_ORDER; i++) {
		list_init(&alloc_state->free_lists[i]);
	}

	// The remainder of fields are correctly initialised as zero values.
}

struct phys_alloc_state *phys_get_alloc_state(void)
{
	return alloc_state;
}
