#include "zeptux.h"

static uint64_t _map_page_range_pud(pudaddr_t pud, virtaddr_t start_va,
				    physaddr_t start_pa, int64_t num_pages,
				    map_flags_t flags,
				    struct page_allocators *alloc)
{
	IGNORE_PARAM(pud);
	IGNORE_PARAM(start_va);
	IGNORE_PARAM(start_pa);
	IGNORE_PARAM(num_pages);
	IGNORE_PARAM(flags);
	IGNORE_PARAM(alloc);

	return 0;
}

uint64_t _map_page_range(pgdaddr_t pgd, virtaddr_t start_va,
			 physaddr_t start_pa, int64_t num_pages,
			 map_flags_t flags, struct page_allocators *alloc)
{
	virtaddr_t va = start_va;
	physaddr_t pa = start_pa;

	uint64_t num_pages_allocated = 0;

	while (num_pages > 0) {
		uint64_t index = virt_pgde_index(va);

		pudaddr_t pud;
		pgde_t pgde = *pgde_at(pgd, index);
		if (!pgde_present(pgde)) {
			pud = alloc->pud();
			num_pages_allocated++;
			assign_pud(pgd, index, pud);
		} else {
			pud = pgde_pud(pgde);
		}
		num_pages_allocated += _map_page_range_pud(
			pud, va, pa, num_pages, flags, alloc);

		int64_t pages = (int64_t)virt_pgde_remaining_pages(va);

		pa = phys_offset_pages(pa, pages);
		va = virt_offset_pages(va, pages);
		num_pages -= pages;
	}

	return num_pages_allocated;
}
