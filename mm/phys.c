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

// Decrement refcount of and maybe free (possibly compound) physblock referred
// to by `block` which is assumed to have its lock held. It releases the lock
// after it is done.
static void free_physblock_locked(struct physblock *block)
{
	if (block->refcount == 0) {
		goto done;
	} else if (block->refcount > 1) {
		block->refcount--;
		goto done;
	} else { // refcount == 1.
		block->refcount = 0;
	}

	// TODO: Rest of implementation.

done:
	spinlock_release(&block->lock);
}

// Free all unallocated memory in the span to the physical page allocator.
static void phys_alloc_init_span(struct phys_alloc_span *span)
{
	pfn_t pfn = {span->start_pfn.x};
	for (uint64_t i = 0; i < span->num_pages; i++, pfn.x++) {
		struct physblock *block = pfn_to_physblock_lock(pfn);

		if (IS_MASK_SET(block->type, PHYSBLOCK_UNALLOC)) {
			// Set refcount -> 1 so the freeing mechanism actually
			// frees the page.
			block->refcount++;
			free_physblock_locked(block);
		} else {
			spinlock_release(&block->lock);
		}
	}
}

void phys_alloc_init(void)
{
	for (uint64_t i = 0; i < alloc_state->num_spans; i++) {
		phys_alloc_init_span(&alloc_state->spans[i]);
	}
}

void phys_free_pfn(pfn_t pfn)
{
	struct physblock *block = pfn_to_physblock_lock(pfn);
	free_physblock_locked(block);
}
