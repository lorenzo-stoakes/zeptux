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

// Map page tables entries at the PTD level for the specified map range.
static void _map_page_range_ptd(ptdaddr_t ptd, struct page_map_state *state)
{
	IGNORE_PARAM(ptd);
	IGNORE_PARAM(state);
}

// Map page tables entries at the PMD level, trying to use 2 MiB pages if
// possible for the specified map range.
static void _map_page_range_pmd(pmdaddr_t pmd, struct page_map_state *state)
{
	// Declare here as we should avoid jumping over a declaration.
	uint64_t pages;

	while (state->num_remaining_pages > 0) {
		uint64_t index = virt_pmde_index(state->va);
		pmde_t pmde = *pmde_at(pmd, index);
		bool present = pmde_present(pmde);

		ptdaddr_t ptd;
		if (!present && IS_ALIGNED(state->va.x, PAGE_SIZE_2MIB) &&
		    IS_ALIGNED(state->pa.x, PAGE_SIZE_2MIB) &&
		    state->num_remaining_pages >= (int64_t)NUM_PAGES_PTD) {
			// PTD not mapped, we can map as 2 MiB page.
			assign_data_2mib(pmd, index, state->pa, state->flags);

			// Move to next PMDE.
			state->pa = phys_offset_pages(state->pa, NUM_PAGES_PTD);
			state->va = virt_offset_pages(state->va, NUM_PAGES_PTD);
			state->num_remaining_pages -= NUM_PAGES_PTD;
			goto next;
		} else if (present && pmde_2mib(pmde)) {
			// 2 MiB data page already mapped, overlapping mappings
			// are not permitted so panic.
			state->alloc->panic(
				"Unable to map VA 0x%lx at PA 0x%lx as 2 MiB PMDE already maps to 0x%lx",
				state->va.x, state->pa.x,
				pmde_data_2mib(pmde).x);
		} else if (!present) {
			// PTD not mapped so we have to allocate.
			ptd = state->alloc->ptd();
			state->num_pagetables_allocated++;
			assign_ptd(pmd, index, ptd);
		} else {
			// PMD mapped.
			ptd = pmde_ptd(pmde);
		}

		_map_page_range_ptd(ptd, state);
		if (state->num_remaining_pages <= 0)
			break;

		// TODO: Full page table assignments not implemented yet, REMOVE.
		break;

		pages = virt_pmde_remaining_pages(state->va);
		if (pages > 0)
			state->alloc->panic(
				"%lu pages unassigned under PMD for VA 0x%lx",
				state->num_remaining_pages, state->va.x);

		// If we just assigned the last entry in the page table, we need
		// a new PUDE.
	next:
		if (index == NUM_PAGE_TABLE_ENTRIES - 1)
			break;
	}
}

// Map page tables entries at the PUD level, trying to use 1 GiB pages if
// possible for the specified map range.
static void _map_page_range_pud(pudaddr_t pud, struct page_map_state *state)
{
	// Declare here as we should avoid jumping over a declaration.
	uint64_t pages;

	while (state->num_remaining_pages > 0) {
		uint64_t index = virt_pude_index(state->va);
		pude_t pude = *pude_at(pud, index);
		bool present = pude_present(pude);

		pmdaddr_t pmd;
		if (!present && IS_ALIGNED(state->va.x, PAGE_SIZE_1GIB) &&
		    IS_ALIGNED(state->pa.x, PAGE_SIZE_1GIB) &&
		    state->num_remaining_pages >= (int64_t)NUM_PAGES_PMD) {
			// PMD not mapped, we can map as 1 GiB page.
			assign_data_1gib(pud, index, state->pa, state->flags);

			// Move to next PUDE.
			state->pa = phys_offset_pages(state->pa, NUM_PAGES_PMD);
			state->va = virt_offset_pages(state->va, NUM_PAGES_PMD);
			state->num_remaining_pages -= NUM_PAGES_PMD;
			goto next;
		} else if (present && pude_1gib(pude)) {
			// 1 GiB data page already mapped, overlapping mappings
			// are not permitted so panic.
			state->alloc->panic(
				"Unable to map VA 0x%lx at PA 0x%lx as 1 GiB PUDE already maps to 0x%lx",
				state->va.x, state->pa.x,
				pude_data_1gib(pude).x);
		} else if (!present) {
			// PMD not mapped so we have to allocate.
			pmd = state->alloc->pmd();
			state->num_pagetables_allocated++;
			assign_pmd(pud, index, pmd);
		} else {
			// PMD mapped.
			pmd = pude_pmd(pude);
		}

		_map_page_range_pmd(pmd, state);
		if (state->num_remaining_pages <= 0)
			break;

		// TODO: Full page table assignments not implemented yet, REMOVE.
		break;

		pages = virt_pude_remaining_pages(state->va);
		if (pages > 0)
			state->alloc->panic(
				"%lu pages unassigned under PUD for VA 0x%lx",
				state->num_remaining_pages, state->va.x);

		// If we just assigned the last entry in the page table, we need
		// a new PGDE.
	next:
		if (index == NUM_PAGE_TABLE_ENTRIES - 1)
			break;
	}
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
		pgde_t pgde = *pgde_at(pgd, index);

		pudaddr_t pud;
		if (!pgde_present(pgde)) {
			pud = state.alloc->pud();
			state.num_pagetables_allocated++;
			assign_pud(pgd, index, pud);
		} else {
			pud = pgde_pud(pgde);
		}
		_map_page_range_pud(pud, &state);
		if (state.num_remaining_pages <= 0)
			break;

		// TODO: Full page table assignments not implemented yet, REMOVE.
		break;

		// If we have pages still to be mapped AND there are pages
		// remaining before the next PGDE then something has gone
		// horribly wrong.
		uint64_t pages = virt_pgde_remaining_pages(state.va);
		if (pages > 0)
			state.alloc->panic(
				"%lu pages unassigned under PGD for VA 0x%lx",
				state.num_remaining_pages, state.va.x);
	}

	return state.num_pagetables_allocated;
}
