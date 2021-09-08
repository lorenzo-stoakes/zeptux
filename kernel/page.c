#include "zeptux.h"

uint64_t _map_page_range(pgdaddr_t pgd, virtaddr_t start_va,
			 physaddr_t start_pa, uint64_t num_pages,
			 struct page_allocators *alloc)
{
	// TODO: Implement.
	IGNORE_PARAM(pgd);
	IGNORE_PARAM(start_va);
	IGNORE_PARAM(start_pa);
	IGNORE_PARAM(num_pages);
	IGNORE_PARAM(alloc);

	return 0;
}
