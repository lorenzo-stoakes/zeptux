#include "test_early.h"

const char *test_phys_alloc(void)
{
	struct phys_alloc_state *state = phys_get_alloc_state_lock();
	// We are single threaded at this point so no need for locks.
	spinlock_release(&state->lock);
	struct phys_alloc_stats *stats = &state->stats;

	int order = MAX_ORDER;
	for (; order >= 0; order--) {
		if (stats->order[order].num_free_pages > 0)
			break;
	}

	assert(order > 0, "No free memory?");

	// We are still in early stage kernel mode, single core and we have only
	// initialised things up to the point of having a physical allocator, so
	// we can rely on only us allocating.

	// Try allocating/freeing from the highest available order page.
	uint64_t num_free_pages = stats->order[order].num_free_pages;
	physaddr_t pa = phys_alloc(order, ALLOC_KERNEL);
	assert(IS_ALIGNED(pa.x, 1UL << order), "Misaligned PA?");
	assert(stats->order[order].num_free_pages == num_free_pages - 1,
	       "Stats not updated after alloc?");
	phys_free(pa);
	assert(stats->order[order].num_free_pages == num_free_pages,
	       "Stats not updated after free?");

	return NULL;
}
