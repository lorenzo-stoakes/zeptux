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

		struct physblock *block = phys_to_physblock_lock(pa);
		assert(block->type == PHYSBLOCK_KERNEL,
		       "Not marked kernel physblock type?");
		assert(block->order == order, "Inocrrect order set?");
		assert(block->refcount == 1, "refcount not set?");
		spinlock_release(&block->lock);

		assert(IS_ALIGNED(pa.x, 1UL << (order + PAGE_SHIFT)),
		       "Misaligned PA?");

		assert(stats->order[order].num_free_pages == num_free_pages - 1,
		       "Stats not updated after alloc?");
		assert(stats->num_free_4k_pages == num_4k_pages - (1UL << order),
		       "4 KiB pages stats not updated after alloc?");

		if (order > 0) {
			for (pfn_t tail_pfn = {phys_to_pfn(pa).x + 1};
			     tail_pfn.x < (1UL << order); tail_pfn.x++) {
				struct physblock *tail_block =
					_pfn_to_physblock_raw(tail_pfn);
				assert(tail_block == block,
				       "Tail PA did not link back to head block");
			}
		}

		phys_free(pa);
		assert(stats->order[order].num_free_pages == num_free_pages,
		       "Stats not updated after free?");
		assert(stats->num_free_4k_pages == num_4k_pages,
		       "4 KiB pages stats not updated after alloc?");
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

	uint8_t next_order = 1;
	for (; next_order <= MAX_ORDER; next_order++) {
		if (!list_empty(&state->free_lists[next_order]))
			break;
	}

	assert(next_order <= MAX_ORDER,
	       "No further free pages at higher orders?");

	uint64_t num_next_order_pages = stats->order[next_order].num_free_pages;

	// Allocate a page, which should split next_order pages down.
	physaddr_t pa = phys_alloc_one();

	assert(stats->order[next_order].num_free_pages ==
		       num_next_order_pages - 1,
	       "Didn't split higher order page?");

	// All other pages above should be split leaving 1 page behind at each
	// level including order 0.
	for (int order = (int)next_order - 1; order >= 0; order--) {
		assert(stats->order[order].num_free_pages == 1,
		       "Did not split correctly");
	}

	struct physblock *block = phys_to_physblock_lock(pa);
	assert((block->type & PHYSBLOCK_TYPE_MASK) == PHYSBLOCK_KERNEL,
	       "Not marked kernel physblock type?");
	assert(block->order == 0, "Incorrect order set?");
	assert(block->refcount == 1, "refcount not set?");
	spinlock_release(&block->lock);

	phys_free(pa);

	for (int order = 0; order < next_order; order++) {
		assert(stats->order[order].num_free_pages == 0,
		       "Did not compact correctly?");
	}
	assert(stats->order[next_order].num_free_pages == num_next_order_pages,
	       "Did not compact correctly?");

	return NULL;
}
