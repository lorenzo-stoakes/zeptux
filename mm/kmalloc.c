#include "zeptux.h"

static uint8_t bytes_to_order(uint64_t bytes)
{
	uint64_t pages = bytes_to_pages(bytes);

	uint64_t max_pages = 1UL << MAX_ORDER;
	if (pages > max_pages)
		panic("Cannot allocate %lu bytes, exceeds maximum of %lu",
		      PAGE_SIZE * pages, PAGE_SIZE * max_pages);

	// TODO: Do this more efficiently.
	for (uint8_t order = 0; order < MAX_ORDER; order++) {
		uint64_t order_pages = 1UL << order;
		if (order_pages >= pages)
			return order;
	}

	panic("Impossible?");
}

// Allocate memory of the specified order and kmalloc flags.
static void *_kmalloc(uint8_t order, kmalloc_flags_t flags)
{
	alloc_flags_t alloc_flags;
	switch (flags & KMALLOC_TYPE_MASK) {
	case KMALLOC_KERNEL:
		alloc_flags = ALLOC_KERNEL | ALLOC_MOVABLE;
		break;
	default:
		panic("Unrecognised kernel flag %d", flags & KMALLOC_TYPE_MASK);
	}

	// Simplistic implementation - just allocate physical pages.
	physaddr_t pa = phys_alloc(order, alloc_flags);
	return phys_to_virt_ptr(pa);
}

void *kmalloc(uint64_t size, kmalloc_flags_t flags)
{
	uint8_t order = bytes_to_order(size);
	return _kmalloc(order, flags);
}

void *kzalloc(uint64_t size, kmalloc_flags_t flags)
{
	void *ret = kmalloc(size, flags);
	memset(ret, 0, size);

	return ret;
}

void kfree(void *ptr)
{
	physaddr_t pa = virt_ptr_to_phys(ptr);
	phys_free(pa);
}
