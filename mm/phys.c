#include "early_mem.h"
#include "zeptux.h"

static struct phys_alloc_state *alloc_state;

// The root kernel PGD.
pgdaddr_t kernel_root_pgd;

void phys_alloc_init_state(void *ptr, struct early_page_alloc_state *early_state)
{
	alloc_state = (struct phys_alloc_state *)ptr;

	for (int i = 0; i <= MAX_ORDER; i++) {
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

struct phys_alloc_state *phys_get_alloc_state_locked(void)
{
	spinlock_acquire(&alloc_state->lock);
	return alloc_state;
}

// Obtain the buddy physblock for a specified physblock if is within available
// memory range, if not returns NULL.
// ASSUMES: `block` has lock held.
// ASSUMES: `alloc_state` has lock held.
static struct physblock *physblock_to_buddy_lock(struct physblock *block)
{
	pfn_t pfn = physblock_to_pfn(block);
	pfn_t buddy_pfn = pfn_to_buddy_pfn(pfn, block->order);

	// Both the PFN and buddy PFN must exist in the same physical span of
	// memory.
	int span = pfn_to_span_locked(pfn);
	int buddy_span = pfn_to_span_locked(buddy_pfn);
	if (span == -1 || span != buddy_span)
		return NULL;

	return pfn_to_physblock_lock(buddy_pfn);
}

// Mark all tail pages of a compound page tail.
// ASSUMES: Appropriate locks are held, all of which are cleared.
static void _set_tail_physblocks(struct physblock *start, uint8_t order)
{
	uint16_t num_blocks = 1 << order;
	for (uint16_t i = 1; i < num_blocks; i++) {
		struct physblock *block = &start[i];

		block->type = PHYSBLOCK_TAIL;
		// The offset to the head page.
		block->head_offset = i;
		// By convention we mark tail pages order 0.
		block->order = 0;

		// If not already held, this is a noop.
		spinlock_release(&block->lock);
	}
}

// Joins two physblocks into 1 of order `order`, updating the free lists accordingly.
// ASSUMES: Both blocks are of PHYSBLOCK_FREE type and equal order.
// ASSUMES: head < tail.
// ASSUMES: Both blocks have locks held. `tail` will have lock cleared by this
// function.
// ASSUMES: `alloc_state` has lock held.
static struct physblock *_join_physblocks_locked(struct physblock *head,
						 struct physblock *tail,
						 uint8_t order)
{
	head->order = order;
	list_detach(&head->node);
	list_detach(&tail->node);
	list_push_back(&alloc_state->free_lists[order], &head->node);
	// Clears all tail locks also.
	_set_tail_physblocks(head, order);

	// General stats do not change, but order ones do!
	struct phys_alloc_stats *stats = &alloc_state->stats;
	stats->order[order - 1].num_free_pages -= 2;
	stats->order[order].num_free_pages++;

	return head;
}

// Compact blocks of memory to as high an order as possible.
// ASSUMES: `block` has lock held.
// ASSUMES: `alloc_state` has lock held.
static struct physblock *compact_free_blocks_locked(struct physblock *block)
{
	for (uint8_t order = block->order; order <= MAX_ORDER - 1; order++) {
		struct physblock *buddy = physblock_to_buddy_lock(block);

		if (buddy == NULL)
			return block;

		if ((buddy->type & PHYSBLOCK_TYPE_MASK) != PHYSBLOCK_FREE ||
		    buddy->order != order) {
			spinlock_release(&buddy->lock);
			return block;
		}

		// The lock on the buddy block will be cleared in this
		// operation. Afterwards, block will of order order+1.
		if (buddy < block)
			block = _join_physblocks_locked(buddy, block, order + 1);
		else
			block = _join_physblocks_locked(block, buddy, order + 1);
	}

	return block;
}

// Decrement refcount of and maybe free (possibly compound) physblock referred
// to by `block` which is assumed to have its lock held. It releases the lock
// after it is done.
// ASSUMES: `block` has lock held.
static void free_physblock_locked(struct physblock *block)
{
	// If already freed or refcount > 1 we don't need to free the page.
	if (block->refcount == 0) {
		goto done;
	} else if (block->refcount > 1) {
		block->refcount--;
		goto done;
	} else {
		block->refcount = 0;
	}

	// Free.
	uint8_t order = block->order;
	spinlock_acquire(&alloc_state->lock);
	if (block->type != PHYSBLOCK_FREE) {
		block->type = PHYSBLOCK_FREE;
		list_push_back(&alloc_state->free_lists[order], &block->node);
		alloc_state->stats.num_free_4k_pages += 1UL << order;
		alloc_state->stats.order[order].num_free_pages++;
	}
	// Compact.
	block = compact_free_blocks_locked(block);
	spinlock_release(&alloc_state->lock);
done:
	spinlock_release(&block->lock);
}

// Free all unallocated memory in the span to the physical page allocator.
// ASSUMES: We are in early single core state.
static void phys_alloc_init_span(struct phys_alloc_span *span)
{
	// NOTE: we do not acquire alloc_state locks here before manipulating
	// stats, this is because we know we are in a single core
	// initialisation. Functions we invoke that are more general will
	// acquire locks as they are designed for general operation. At this
	// stage they're not necesasry but not harmful.

	struct phys_alloc_stats *stats = &alloc_state->stats;

	stats->num_4k_pages += span->num_pages;

	pfn_t pfn = {span->start_pfn.x};
	for (uint64_t i = 0; i < span->num_pages; i++, pfn.x++) {
		struct physblock *block = pfn_to_physblock_lock(pfn);

		if ((block->type & PHYSBLOCK_TYPE_MASK) == PHYSBLOCK_UNMANAGED) {
			// Set refcount -> 1 so the freeing mechanism actually
			// frees the page.
			block->refcount = 1;

			free_physblock_locked(block);
		} else {
			if ((block->type & PHYSBLOCK_TYPE_MASK) ==
			    PHYSBLOCK_PAGETABLE)
				stats->num_pagetable_pages++;
			else if ((block->type & PHYSBLOCK_TYPE_MASK) ==
				 PHYSBLOCK_PHYSBLOCK)
				stats->num_physblock_pages++;

			spinlock_release(&block->lock);
		}
	}
}

// Split physblock `block` into 2 blocks of order - 1 and place the 2 halves
// into the appropriate free list.
// ASSUMES: `alloc_state` has lock held.
// ASSUMES: `block` has lock held.
static void split_block_locked(struct physblock *block)
{
	struct phys_alloc_stats *stats = &alloc_state->stats;
	uint8_t new_order = block->order - 1;
	pfn_t pfn = physblock_to_pfn(block);
	pfn_t buddy_pfn = pfn_to_buddy_pfn(pfn, new_order);
	struct physblock *buddy = _pfn_to_physblock_raw(buddy_pfn);

	block->order = new_order;
	buddy->order = new_order;
	buddy->type = PHYSBLOCK_FREE;

	// All tail pages will already be marked tail with correct offsets for
	// `block` but `buddy` will need to have head_offset values reset.
	_set_tail_physblocks(buddy, new_order);

	list_detach(&block->node);
	struct list *free_list = &alloc_state->free_lists[new_order];
	list_push_back(free_list, &block->node);
	list_push_back(free_list, &buddy->node);

	stats->order[new_order + 1].num_free_pages--;
	stats->order[new_order].num_free_pages += 2;

	spinlock_release(&block->lock);
}

// Split higher order physblocks in order to free up a physblock of order
// `order`. Panics if unable to do so.
// ASSUME: `alloc_state` has lock held.
static void split_higher_order_blocks(uint8_t target_order)
{
	// Find the first non-empty free list.
	uint8_t order = target_order + 1;
	for (; order <= MAX_ORDER; order++) {
		if (!list_empty(&alloc_state->free_lists[order]))
			break;
	}

	if (order > MAX_ORDER)
		panic("Out of memory (fragmentation), cannot split pages to obtain one of order %u",
		      target_order);

	// Now start splitting blocks.
	for (; order >= target_order + 1; order--) {
		struct physblock *block = list_first_element(
			&alloc_state->free_lists[order], struct physblock, node);
		spinlock_acquire(&block->lock);

		// If somehow the block got swiped from under us, try again.
		if (block->type != PHYSBLOCK_FREE || block->order != order) {
			spinlock_release(&block->lock);
			order++;
			continue;
		}

		// Will handle locks.
		split_block_locked(block);
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

int pfn_to_span_locked(pfn_t pfn)
{
	for (int i = 0; i < (int)alloc_state->num_spans; i++) {
		struct phys_alloc_span *span = &alloc_state->spans[i];
		pfn_t start = span->start_pfn;

		if (pfn.x >= start.x && pfn.x < start.x + span->num_pages)
			return i;
	}

	return -1;
}

// Convert allocation flags to physblock type.
static inline physblock_type_t alloc_flags_to_physblock_type(alloc_flags_t flags)
{
	physblock_type_t type;
	switch (flags & ALLOC_TYPE_MASK) {
	case ALLOC_KERNEL:
		type = PHYSBLOCK_KERNEL;
		break;
	case ALLOC_USER:
		type = PHYSBLOCK_USER;
		break;
	case ALLOC_PAGETABLE:
		type = PHYSBLOCK_PAGETABLE;
		break;
	case ALLOC_PHYSBLOCK:
		type = PHYSBLOCK_PHYSBLOCK;
		break;
	default:
		panic("Unrecognised alloc flag %d", flags & ALLOC_TYPE_MASK);
	}

	if (IS_MASK_SET(flags, ALLOC_MOVABLE))
		type |= PHYSBLOCK_MOVABLE;
	if (IS_MASK_SET(flags, ALLOC_PINNED))
		type |= PHYSBLOCK_PINNED;

	return type;
}

struct physblock *phys_alloc_block(uint8_t order, alloc_flags_t flags)
{
	if (order > MAX_ORDER)
		panic("Invalid order %u, maximum is %u", order, MAX_ORDER);

	spinlock_acquire(&alloc_state->lock);

	struct phys_alloc_stats *stats = &alloc_state->stats;

	uint64_t num_4k_pages = 1UL << order;
	if (stats->num_free_4k_pages < num_4k_pages)
		panic("Out of memory: %lu pages requested, %lu available (order %u)",
		      num_4k_pages, stats->num_free_4k_pages, order);

	struct list *free_list = &alloc_state->free_lists[order];

	// If we don't have enough pages available at the requested order we
	// have to split larger order pages to obtain one. If we cannot this will panic.
	if (list_empty(free_list))
		split_higher_order_blocks(order);

	struct physblock *block =
		list_first_element(free_list, struct physblock, node);

	spinlock_acquire(&block->lock);
	// While any actual allocation/freeing will need to acquire an
	// alloc_state lock which we already hold, it's conceivable that, as we
	// didn't previously hold a lock on this block that some future change
	// might result in this being swiped from under us somehow. If so we
	// simply recurse + try again.
	if (block->type != PHYSBLOCK_FREE || block->order != order) {
		spinlock_release(&block->lock);
		spinlock_release(&alloc_state->lock);
		return phys_alloc_block(order, flags);
	}

	list_detach(&block->node);
	block->type = alloc_flags_to_physblock_type(flags);
	block->refcount++;

	stats->num_free_4k_pages -= num_4k_pages;
	stats->order[order].num_free_pages--;

	spinlock_release(&block->lock);
	spinlock_release(&alloc_state->lock);

	return block;
}
