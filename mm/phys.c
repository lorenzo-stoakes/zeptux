#include "early_mem.h"
#include "zeptux.h"

static struct phys_alloc_state *alloc_state;

// The root kernel PGD.
pgdaddr_t kernel_root_pgd;

void phys_alloc_init_state(void *ptr, struct early_page_alloc_state *early_state)
{
	alloc_state = (struct phys_alloc_state *)ptr;

	for (int i = 0; i < MAX_ORDER; i++) {
		list_init(&alloc_state->free_lists[i]);
	}

	// We assume caller has checked to ensure the single page we've been
	// handled is large enough to contain all spans.

	alloc_state->num_spans = early_state->num_spans;
	for (uint64_t i = 0; i < early_state->num_spans; i++) {
		struct early_page_alloc_span *early_span = &early_state->spans[i];
		struct phys_alloc_span *span = &alloc_state->spans[i];

		span->start_pfn = phys_to_pfn(early_span->start);
		span->num_pages = early_span->num_pages;
	}

	// The remainder of fields are correctly initialised as zero values.
}

struct phys_alloc_state *phys_get_alloc_state(void)
{
	return alloc_state;
}
