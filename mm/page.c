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
	while (state->num_remaining_pages > 0) {
		uint64_t index = virt_ptde_index(state->va);
		ptde_t ptde = *ptde_at(ptd, index);

		if (ptde_present(ptde)) {
			if (IS_MASK_SET(state->flags, MAP_SKIP_IF_MAPPED))
				goto next;

			state->alloc->panic(
				"Trying to map VA 0x%lx to PA 0x%lx but already mapped to 0x%lx",
				state->va.x, state->pa.x, ptde_data(ptde).x);
		}

		assign_data(ptd, index, state->pa, state->flags);

	next:
		state->pa = phys_offset_pages(state->pa, 1);
		state->va = virt_offset_pages(state->va, 1);
		state->num_remaining_pages--;

		// If we just assigned the last entry in the page table, we need
		// a new PMDE.
		if (index == NUM_PAGE_TABLE_ENTRIES - 1)
			break;
	}
}

// Map page tables entries at the PMD level, trying to use 2 MiB pages if
// possible for the specified map range.
static void _map_page_range_pmd(pmdaddr_t pmd, struct page_map_state *state)
{
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
				state->va.x, state->pa.x, pmde_data_2mib(pmde).x);
		} else if (!present) {
			// PTD not mapped so we have to allocate.
			ptd = state->alloc->ptd();
			state->num_pagetables_allocated++;
			assign_ptd(pmd, index, ptd);
		} else {
			// PTD mapped.
			ptd = pmde_ptd(pmde);
		}

		_map_page_range_ptd(ptd, state);
		if (state->num_remaining_pages <= 0)
			break;

		if (!IS_ALIGNED(state->va.x, PAGE_SIZE_2MIB))
			state->alloc->panic(
				"%lu pages unassigned under PMD for VA 0x%lx",
				state->num_remaining_pages, state->va.x);

	next:
		// If we just assigned the last entry in the page table, we need
		// a new PUDE.
		if (index == NUM_PAGE_TABLE_ENTRIES - 1)
			break;
	}
}

// Map page tables entries at the PUD level, trying to use 1 GiB pages if
// possible for the specified map range.
static void _map_page_range_pud(pudaddr_t pud, struct page_map_state *state)
{
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
				state->va.x, state->pa.x, pude_data_1gib(pude).x);
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

		if (!IS_ALIGNED(state->va.x, PAGE_SIZE_1GIB))
			state->alloc->panic(
				"%lu pages unassigned under PUD for VA 0x%lx",
				state->num_remaining_pages, state->va.x);

	next:
		// If we just assigned the last entry in the page table, we need
		// a new PGDE.
		if (index == NUM_PAGE_TABLE_ENTRIES - 1)
			break;
	}
}

uint64_t _map_page_range(pgdaddr_t pgd, virtaddr_t start_va, physaddr_t start_pa,
			 int64_t num_pages, map_flags_t flags,
			 struct page_allocators *alloc)
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

		// If we have pages still to be mapped AND we are not aligned to
		// the PGD then something has gone horribly wrong.
		if (!IS_ALIGNED(state.va.x, PGD_SIZE))
			state.alloc->panic(
				"%lu pages unassigned under PGD for VA 0x%lx",
				state.num_remaining_pages, state.va.x);
	}

	return state.num_pagetables_allocated;
}

// Walks page table entries from specified PGD and obtains entry pointing at
// data page. Outputs level obtained from in `level_out`.
static uint64_t walk_to_data(pgdaddr_t pgd, virtaddr_t va,
			     struct page_allocators *alloc,
			     page_level_t *level_out)
{
	pgde_t pgde = *pgde_at(pgd, virt_pgde_index(va));
	if (!pgde_present(pgde))
		alloc->panic("0x%lx: PGDE not present", va.x);

	pudaddr_t pud = pgde_pud(pgde);
	pude_t pude = *pude_at(pud, virt_pude_index(va));
	if (!pude_present(pude))
		alloc->panic("0x%lx: PUDE not present", va.x);

	if (pude_1gib(pude)) {
		*level_out = PUD;
		return pude.x;
	}

	pmdaddr_t pmd = pude_pmd(pude);
	pmde_t pmde = *pmde_at(pmd, virt_pmde_index(va));
	if (!pmde_present(pmde))
		alloc->panic("0x%lx: PMDE not present", va.x);

	if (pmde_2mib(pmde)) {
		*level_out = PMD;
		return pmde.x;
	}

	ptdaddr_t ptd = pmde_ptd(pmde);
	ptde_t ptde = *ptde_at(ptd, virt_ptde_index(va));
	if (!ptde_present(ptde))
		alloc->panic("0x%lx: PTDE not present", va.x);

	*level_out = PTD;
	return ptde.x;
}

uint64_t _walk_virt_to_raw_flags(pgdaddr_t pgd, virtaddr_t va,
				 struct page_allocators *alloc)
{
	page_level_t level;
	uint64_t raw = walk_to_data(pgd, va, alloc, &level);

	switch (level) {
	case PUD:
	{
		pude_t pude = {raw};
		return pude_raw_flags_1gib(pude);
	}
	case PMD:
	{
		pmde_t pmde = {raw};
		return pmde_raw_flags_2mib(pmde);
	}
	case PTD:
	{
		ptde_t ptde = {raw};
		return ptde_raw_flags(ptde);
	}
	default:
		alloc->panic("Bug in walk_to_data()");
	}
}

physaddr_t _walk_virt_to_phys(pgdaddr_t pgd, virtaddr_t va,
			      struct page_allocators *alloc)
{
	page_level_t level;
	uint64_t raw = walk_to_data(pgd, va, alloc, &level);

	switch (level) {
	case PUD:
	{
		pude_t pude = {raw};
		return pude_data_1gib(pude);
	}
	case PMD:
	{
		pmde_t pmde = {raw};
		return pmde_data_2mib(pmde);
	}
	case PTD:
	{
		ptde_t ptde = {raw};
		return ptde_data(ptde);
	}
	default:
		alloc->panic("Bug in walk_to_data()");
	}
}

// Part of the page dump implementation - Stores state during page table dump.
struct page_dump_state {
	bool in_range, mask_huge_flag;
	virtaddr_t start, end;
	uint64_t raw_flags;
	int pgd_index, pud_index, pmd_index;
	uint64_t num_pages_4kib, num_pages_2mib, num_pages_1gib;

	PRINTF(1, 2) int (*printf)(const char *fmt, ...);
};

// Part of the page dump implementation - output page table entry flags.
static void dump_flags(struct page_dump_state *state)
{
	uint64_t flags = state->raw_flags;

	if (IS_MASK_SET(flags, PAGE_FLAG_RW))
		state->printf("RW ");
	else
		state->printf("RO ");

	if (IS_MASK_SET(flags, PAGE_FLAG_USER))
		state->printf("Us ");

	if (IS_MASK_SET(flags, PAGE_FLAG_WRITE_THROUGH))
		state->printf("Wt ");

	if (IS_MASK_SET(flags, PAGE_FLAG_UNCACHED))
		state->printf("Uc ");

	if (IS_MASK_SET(flags, PAGE_FLAG_ACCESSED))
		state->printf("Ax ");

	if (IS_MASK_SET(flags, PAGE_FLAG_DIRTY))
		state->printf("Dt ");

	// Gigantic/huge page.
	if (IS_MASK_SET(flags, PAGE_FLAG_PSE))
		state->printf("Hg ");

	if (IS_MASK_SET(flags, PAGE_FLAG_GLOBAL))
		state->printf("Gl ");

	if (!IS_MASK_SET(flags, PAGE_FLAG_NX))
		state->printf("EX ");
}

// Part of the page dump implementation - dump a memory range using the provided
// printf() function as we are at a discontinuity in the mapping unless we are not
// currently recording a range.
static void maybe_dump_range(struct page_dump_state *state)
{
	if (!state->in_range)
		return;

	state->in_range = false;

	// Sign-extend VAs.
	if (IS_BIT_SET(state->start.x, PHYS_ADDR_BITS - 1)) {
		uint64_t mask = BIT_MASK_ABOVE(PHYS_ADDR_BITS);
		state->start.x |= mask;
		state->end.x |= mask;
	}

	char buf[1024];
	state->printf("0x%016lx - 0x%016lx: %s / ", state->start.x, state->end.x,
		      bytes_to_human(state->end.x - state->start.x, buf,
				     sizeof(buf)));
	dump_flags(state);
	state->printf("\n");

	state->start.x = 0;
	state->end.x = 0;
	state->raw_flags = 0;
}

// Part of the page dump implementation - extend the page table range we are
// storing for the purposes of merging memory ranges for more digestible
// output. We separate the range if the flags differ (possibly not if the only
// differing flag is the huge/gigantic page flag PSE depending on whether the
// 'mask huge flag' option is set).
static void extend_dump_range(struct page_dump_state *state, virtaddr_t start,
			      virtaddr_t end, uint64_t raw_flags)
{
	uint64_t prev_flags = state->raw_flags;

	if (state->mask_huge_flag) {
		uint64_t mask = BIT_MASK_EXCLUDING(PAGE_FLAG_PSE_BIT);

		prev_flags &= mask;
		raw_flags &= mask;
	}

	// If flags mismatch, this is a break in range.
	if (state->in_range && raw_flags != prev_flags) {
		// Force dump of previous range.
		maybe_dump_range(state);
		state->in_range = false;

		// We will invoke the new range logic below to start this new
		// range.
	}

	// Starting new range.
	if (!state->in_range) {
		state->start = start;
		state->end = end;
		state->raw_flags = raw_flags;

		state->in_range = true;
		return;
	}

	// Otherwise we are extending the range.
	state->end = end;
}

// Dump page table entries from PTD level.
static void dump_mapped_pages_ptd(ptdaddr_t ptd, struct page_dump_state *state)
{
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		ptde_t ptde = *ptde_at(ptd, i);

		if (!ptde_present(ptde)) {
			maybe_dump_range(state);
			continue;
		}

		virtaddr_t start = encode_virt(state->pgd_index, state->pud_index,
					       state->pmd_index, i, 0);
		virtaddr_t end = {start.x + PAGE_SIZE};
		extend_dump_range(state, start, end, ptde_raw_flags(ptde));
		state->num_pages_4kib++;
	}
}

// Dump page table entries from PMD level.
static void dump_mapped_pages_pmd(pmdaddr_t pmd, struct page_dump_state *state)
{
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		state->pmd_index = i;

		pmde_t pmde = *pmde_at(pmd, i);

		if (!pmde_present(pmde)) {
			maybe_dump_range(state);
			continue;
		}

		if (pmde_2mib(pmde)) {
			virtaddr_t start = encode_virt(state->pgd_index,
						       state->pud_index,
						       state->pmd_index, 0, 0);
			virtaddr_t end = {start.x + PAGE_SIZE_2MIB};
			extend_dump_range(state, start, end,
					  pmde_raw_flags_2mib(pmde));
			state->num_pages_2mib++;
		} else {
			ptdaddr_t ptd = pmde_ptd(pmde);
			dump_mapped_pages_ptd(ptd, state);
		}
	}
}

// Dump page table entries from PUD level.
static void dump_mapped_pages_pud(pudaddr_t pud, struct page_dump_state *state)
{
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		state->pud_index = i;

		pude_t pude = *pude_at(pud, i);

		if (!pude_present(pude)) {
			maybe_dump_range(state);
			continue;
		}

		if (pude_1gib(pude)) {
			virtaddr_t start = encode_virt(state->pgd_index,
						       state->pud_index, 0, 0, 0);
			virtaddr_t end = {start.x + PAGE_SIZE_1GIB};
			extend_dump_range(state, start, end,
					  pude_raw_flags_1gib(pude));
			state->num_pages_1gib++;
		} else {
			pmdaddr_t pmd = pude_pmd(pude);
			dump_mapped_pages_pmd(pmd, state);
		}
	}
}

void dump_mapped_pages(pgdaddr_t pgd, bool mask_huge_flag,
		       int (*printf)(const char *fmt, ...))
{
	struct page_dump_state state = {.printf = printf,
					.mask_huge_flag = mask_huge_flag};

	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		state.pgd_index = i;

		pgde_t pgde = *pgde_at(pgd, i);

		if (!pgde_present(pgde)) {
			maybe_dump_range(&state);
			continue;
		}

		pudaddr_t pud = pgde_pud(pgde);
		dump_mapped_pages_pud(pud, &state);
	}
	maybe_dump_range(&state);

	uint64_t total_bytes = 0;
	total_bytes += state.num_pages_4kib * PAGE_SIZE;
	total_bytes += state.num_pages_2mib * PAGE_SIZE_2MIB;
	total_bytes += state.num_pages_1gib * PAGE_SIZE_1GIB;

	char buf[1024];
	printf("\n4k[% 5lu] 2m[% 5lu] 1g[% 5lu] % 20s\n", state.num_pages_4kib,
	       state.num_pages_2mib, state.num_pages_1gib,
	       bytes_to_human(total_bytes, buf, sizeof(buf)));
}
