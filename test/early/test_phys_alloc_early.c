#include "test_early.h"

const char *test_phys_alloc(void)
{
	struct phys_alloc_state *state = phys_get_alloc_state_lock();
	// We are single threaded at this point so no need for locks.
	spinlock_release(&state->lock);
	struct phys_alloc_stats *stats = &state->stats;

	for (uint8_t order = 0; order < MAX_ORDER; order++) {
		uint64_t count = list_count(&state->free_lists[order]);
		assert(count == stats->order[order].num_free_pages,
		       "Mismatch between stats and list count?");
	}

	int max_order = MAX_ORDER;
	for (; max_order >= 0; max_order--) {
		if (stats->order[max_order].num_free_pages > 0)
			break;
	}

	assert(max_order > 0, "No free memory?");

	// We are still in early stage kernel mode, single core and we have only
	// initialised things up to the point of having a physical allocator, so
	// we can rely on only us allocating.

	// Try allocating/freeing from the highest available order page to the
	// lowest with available memory. We test splitting/compacting later.
	for (int order = max_order; order >= 0; order--) {
		uint64_t num_free_pages = stats->order[order].num_free_pages;
		uint64_t num_4k_pages = stats->num_free_4k_pages;

		// Skip cases which would require splitting.
		if (num_free_pages == 0)
			continue;

		physaddr_t pa = phys_alloc(order, ALLOC_KERNEL);

		assert(IS_ALIGNED(pa.x, 1UL << (order + PAGE_SHIFT)),
		       "Misaligned PA?");

		assert(stats->order[order].num_free_pages == num_free_pages - 1,
		       "Stats not updated after alloc?");
		assert(stats->num_free_4k_pages == num_4k_pages - (1UL << order),
		       "4 KiB pages stats not updated after alloc?");
		phys_free(pa);
		assert(stats->order[order].num_free_pages == num_free_pages,
		       "Stats not updated after free?");
	}

	// Now allocate every available 4 KiB page.
	uint64_t num_order0_pages = stats->order[0].num_free_pages;
	assert(num_order0_pages > 0, "Not enough order 0 pages to test?");

	uint64_t prev = num_order0_pages;
	for (uint64_t i = 0; i < num_order0_pages; i++, prev--) {
		physaddr_t pa = phys_alloc_one();
		assert(stats->order[0].num_free_pages == prev - 1,
		       "Allocation not update stats?");

		assert(IS_ALIGNED(pa.x, PAGE_SIZE),
		       "4 KiB page size not aligned");
	}

	return NULL;
}
