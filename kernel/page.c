#include "zeptux.h"

// Represents page mapping state, updated as we proceed through the mapping
// process.
struct page_map_state {
	virtaddr_t va;
	physaddr_t pa;
	// Signed so we can advance > num pages + exit early.
	int64_t num_remaining_pages;
	uint64_t num_pagetables_allocated;

	// Immutable:
	map_flags_t flags;
	struct page_allocators *alloc;
};

static void _map_page_range_pud(pudaddr_t pud, struct page_map_state *state)
{
	IGNORE_PARAM(pud);
	IGNORE_PARAM(state);
}

uint64_t _map_page_range(pgdaddr_t pgd, virtaddr_t start_va,
			 physaddr_t start_pa, int64_t num_pages,
			 map_flags_t flags, struct page_allocators *alloc)
{
	struct page_map_state state = {
		.va = start_va,
		.pa = start_pa,
		.num_remaining_pages = num_pages,
		.num_pagetables_allocated = 0,

		.flags = flags,
		.alloc = alloc,
	};

	while (state.num_remaining_pages > 0) {
		uint64_t index = virt_pgde_index(state.va);

		pudaddr_t pud;
		pgde_t pgde = *pgde_at(pgd, index);
		if (!pgde_present(pgde)) {
			pud = state.alloc->pud();
			state.num_pagetables_allocated++;
			assign_pud(pgd, index, pud);
		} else {
			pud = pgde_pud(pgde);
		}
		_map_page_range_pud(pud, &state);

		int64_t pages = (int64_t)virt_pgde_remaining_pages(state.va);

		state.pa = phys_offset_pages(state.pa, pages);
		state.va = virt_offset_pages(state.va, pages);
		state.num_remaining_pages -= pages;
	}

	return state.num_pagetables_allocated;
}
