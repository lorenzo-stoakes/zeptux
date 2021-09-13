#include "test_early.h"

static const char *assert_correct_virtaddr(void)
{
	uint64_t offset = 0;

	// PGD index.
	offset |= 0xde;
	offset <<= 9;
	// PUD index.
	offset |= 0xad;
	offset <<= 9;
	// PMD index.
	offset |= 0xbe;
	offset <<= 9;
	// PTD index.
	offset |= 0xef;
	offset <<= 12;
	// Data page offset.
	offset |= 1234;

	virtaddr_t va = {offset};

	assert(virt_data_offset(va) == 1234, "data offset != 1234");
	assert(virt_ptde_index(va) == 0xef, "ptde index != 0xef");
	assert(virt_pmde_index(va) == 0xbe, "pmde index != 0xbe");
	assert(virt_pude_index(va) == 0xad, "pude index != 0xad");
	assert(virt_pgde_index(va) == 0xde, "pgde index != 0xde");

	assert(encode_virt(0xde, 0xad, 0xbe, 0xef, 0x4d2).x == va.x,
	       "encode_virt() not correctly encoding");

	physaddr_t pa = {0xdeadbeef};
	va = phys_to_virt(pa);
	assert(va.x == KERNEL_DIRECT_MAP_BASE + 0xdeadbeef,
	       "phys_to_virt() not giving correct direct map address");

	void *ptr = phys_to_virt_ptr(pa);
	assert((uint64_t)ptr == KERNEL_DIRECT_MAP_BASE + 0xdeadbeef,
	       "phys_to_virt_ptr() not giving correct direct map address");

	assert(virt_to_phys(va).x == 0xdeadbeef,
	       "Converting virtual address back to physical failed");

	// We also need to test that we can convert a virtual address from the
	// kernel ELF to a physical address correctly.
	va.x = KERNEL_ELF_ADDRESS + 0xbeef;
	assert(virt_to_phys(va).x == 0xbeef,
	       "virt_to_phys() not decoded ELF VA");

	ptr = (void *)va.x;
	assert(virt_ptr_to_phys(ptr).x == 0xbeef,
	       "virt_ptr_to_phys() not decoded ELF VA");

	va = encode_virt(123, 5, 6, 507, 15);
	va = virt_offset_pages(va, 15);
	assert(virt_pgde_index(va) == 123,
	       "virt_offset_pages() incorrect pgde index");
	assert(virt_pude_index(va) == 5,
	       "virt_offset_pages() incorrect pude index");
	assert(virt_pmde_index(va) == 7,
	       "virt_offset_pages() incorrect pmde index");
	assert(virt_ptde_index(va) == 10,
	       "virt_offset_pages() incorrect ptde index");
	assert(virt_data_offset(va) == 0,
	       "virt_offset_pages() did not zero data offset");

	// PGDE:

	va = encode_virt(123, 0, 0, 0, 0);
	assert(virt_pgde_remaining_pages(va) == NUM_PAGES_PUD,
	       "virt_pgde_remaining_pages() incorrect for full PGDE range");
	assert(virt_pgde_index(virt_next_pgde(va)) == 124,
	       "virt_next_pgde() not correctly incrementing");

	va = encode_virt(123, 0, 0, 0, 4095);
	assert(virt_pgde_remaining_pages(va) == NUM_PAGES_PUD,
	       "virt_pgde_remaining_pages() incorrect for offset full PGDE range");
	assert(virt_pgde_index(virt_next_pgde(va)) == 124,
	       "virt_next_pgde() not correctly incrementing");

	va = encode_virt(123, 511, 0, 0, 0);
	assert(virt_pgde_remaining_pages(va) == NUM_PAGES_PMD,
	       "virt_pgde_remaining_pages() incorrect for 1 PUD remaining");
	assert(virt_pgde_index(virt_next_pgde(va)) == 124,
	       "virt_next_pgde() not correctly incrementing");

	va = encode_virt(123, 511, 511, 0, 0);
	assert(virt_pgde_remaining_pages(va) == NUM_PAGES_PTD,
	       "virt_pgde_remaining_pages() incorrect for 1 PMD remaining");
	assert(virt_pgde_index(virt_next_pgde(va)) == 124,
	       "virt_next_pgde() not correctly incrementing");

	va = encode_virt(123, 511, 511, 511, 0);
	assert(virt_pgde_remaining_pages(va) == 1,
	       "virt_pgde_remaining_pages() incorrect for 1 page remaining");
	assert(virt_pgde_index(virt_next_pgde(va)) == 124,
	       "virt_next_pgde() not correctly incrementing");

	// PUDE:

	va = encode_virt(123, 0, 0, 0, 0);
	assert(virt_pude_remaining_pages(va) == NUM_PAGES_PMD,
	       "virt_pude_remaining_pages() incorrect for full PUDE range");
	assert(virt_pude_index(virt_next_pude(va)) == 1,
	       "virt_next_pude() not correctly incrementing");

	va = encode_virt(123, 0, 0, 0, 4095);
	assert(virt_pude_remaining_pages(va) == NUM_PAGES_PMD,
	       "virt_pude_remaining_pages() incorrect for offset full PUDE range");
	assert(virt_pude_index(virt_next_pude(va)) == 1,
	       "virt_next_pude() not correctly incrementing");

	va = encode_virt(123, 1, 0, 0, 0);
	assert(virt_pude_remaining_pages(va) == NUM_PAGES_PMD,
	       "virt_pude_remaining_pages() incorrect for full PUDE range");
	assert(virt_pude_index(virt_next_pude(va)) == 2,
	       "virt_next_pude() not correctly incrementing");

	va = encode_virt(123, 511, 511, 0, 0);
	assert(virt_pude_remaining_pages(va) == NUM_PAGES_PTD,
	       "virt_pude_remaining_pages() incorrect for 1 PMD remaining");
	assert(virt_pude_index(virt_next_pude(va)) == 0,
	       "virt_next_pude() not correctly incrementing");

	va = encode_virt(123, 511, 511, 511, 0);
	assert(virt_pude_remaining_pages(va) == 1,
	       "virt_pude_remaining_pages() incorrect for 1 page remaining");
	assert(virt_pude_index(virt_next_pude(va)) == 0,
	       "virt_next_pude() not correctly incrementing");

	// PMDE:

	va = encode_virt(123, 0, 0, 0, 0);
	assert(virt_pmde_remaining_pages(va) == NUM_PAGES_PTD,
	       "virt_pmde_remaining_pages() incorrect for full PMDE range");
	assert(virt_pmde_index(virt_next_pmde(va)) == 1,
	       "virt_next_pmde() not correctly incrementing");

	va = encode_virt(123, 0, 0, 0, 4095);
	assert(virt_pmde_remaining_pages(va) == NUM_PAGES_PTD,
	       "virt_pmde_remaining_pages() incorrect for offset full PMDE range");
	assert(virt_pmde_index(virt_next_pmde(va)) == 1,
	       "virt_next_pmde() not correctly incrementing");

	va = encode_virt(123, 0, 1, 0, 0);
	assert(virt_pmde_remaining_pages(va) == NUM_PAGES_PTD,
	       "virt_pmde_remaining_pages() incorrect for full PMDE range");
	assert(virt_pmde_index(virt_next_pmde(va)) == 2,
	       "virt_next_pmde() not correctly incrementing");

	va = encode_virt(123, 511, 511, 0, 0);
	assert(virt_pmde_remaining_pages(va) == NUM_PAGES_PTD,
	       "virt_pmde_remaining_pages() incorrect for 1 PMD remaining");
	assert(virt_pmde_index(virt_next_pmde(va)) == 0,
	       "virt_next_pmde() not correctly incrementing");

	va = encode_virt(123, 511, 511, 511, 0);
	assert(virt_pmde_remaining_pages(va) == 1,
	       "virt_pmde_remaining_pages() incorrect for 1 page remaining");
	assert(virt_pmde_index(virt_next_pmde(va)) == 0,
	       "virt_next_pmde() not correctly incrementing");

	return NULL;
}

static const char *assert_page_misc(void)
{
	assert(bytes_to_pages(0) == 0, "bytes_to_pages(0) != 0");
	assert(bytes_to_pages(1) == 1,
	       "1 byte requires at least 1 page to store");
	assert(bytes_to_pages(PAGE_SIZE) == 1,
	       "PAGE_SIZE byte requires at least 1 page to store");
	assert(bytes_to_pages(PAGE_SIZE + 1) == 2,
	       "PAGE_SIZE + 1 byte requires at least 2 pages to store");

	physaddr_t pa = early_page_alloc();

	uint8_t *ptr = phys_to_virt_ptr(pa);
	for (int i = 0; i < (int)PAGE_SIZE; i++) {
		ptr[i] = 123;
	}
	zero_page(pa);

	for (int i = 0; i < (int)PAGE_SIZE; i++) {
		assert(ptr[i] == 0, "Page not zeroed");
	}

	early_page_free(pa);

	pa.x = 0xabcd;
	assert(pa_next_page(pa).x == 0xb000,
	       "Page next not advancing to next page");
	pa.x = 0xafff;
	assert(pa_next_page(pa).x == 0xb000,
	       "Page next not advancing to next page");
	pa.x = 0xa000;
	assert(pa_next_page(pa).x == 0xb000,
	       "Page next not advancing to next page");

	pa.x = 0xabcd;
	assert(pa_prev_page(pa).x == 0x9000,
	       "Page prev not advancing to previous page");
	pa.x = 0xafff;
	assert(pa_prev_page(pa).x == 0x9000,
	       "Page prev not advancing to previous page");
	pa.x = 0xa000;
	assert(pa_prev_page(pa).x == 0x9000,
	       "Page prev not advancing to previous page");

	pa.x = 0xabcd;
	assert(pa_to_pfn(pa).x == 0xa,
	       "pa_to_pfn() not correctly counting PA index");
	pa.x = 0xa000;
	assert(pa_to_pfn(pa).x == 0xa,
	       "pa_to_pfn() not correctly counting PA index");
	pa.x = 0xafff;
	assert(pa_to_pfn(pa).x == 0xa,
	       "pa_to_pfn() not correctly counting PA index");
	pa.x = 0xb123;
	assert(pa_to_pfn(pa).x == 0xb,
	       "pa_to_pfn() not correctly counting PA index");

	pfn_t pfn = {0xa};
	assert(pfn_to_pa(pfn).x == 0xa000,
	       "pfn_to_pa() not correctly converting PFN to PA");
	pfn.x = 0xb;
	assert(pfn_to_pa(pfn).x == 0xb000,
	       "pfn_to_pa() not correctly converting PFN to PA");
	pfn.x = 0;
	assert(pfn_to_pa(pfn).x == 0,
	       "pfn_to_pa() not correctly converting PFN to PA");

	pa.x = 117 << PAGE_SHIFT;
	// Add some data offset noise.
	pa.x |= 4095;
	assert(phys_offset_pages(pa, 12).x >> PAGE_SHIFT == 129,
	       "phys_offset_pages() not offsetting correctly");

	return NULL;
}

static const char *assert_pagetable_helpers(void)
{
	pgde_t pgde = {0xbeef0};
	assert(!pgde_present(pgde), "PGDE present should not be set");
	pgde.x |= PAGE_FLAG_PRESENT;
	assert(pgde_present(pgde), "PGDE present should be set");

	pude_t pude = {0xbeef0};
	assert(!pude_present(pude), "PUDE present should not be set");
	pude.x |= PAGE_FLAG_PRESENT;
	assert(pude_present(pude), "PUDE present should be set");

	pmde_t pmde = {0xbeef0};
	assert(!pmde_present(pmde), "PMDE present should not be set");
	pmde.x |= PAGE_FLAG_PRESENT;
	assert(pmde_present(pmde), "PMDE present should be set");

	ptde_t ptde = {0xbeef0};
	assert(!ptde_present(ptde), "PTDE present should not be set");
	ptde.x |= PAGE_FLAG_PRESENT;
	assert(ptde_present(ptde), "PTDE present should be set");

	pgdaddr_t pgd = early_alloc_pgd();
	uint64_t *ptr = pgd_to_virt_ptr(pgd);
	// Fill for later tests.
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		ptr[i] = i + 1;
	}
	assert((uint64_t)ptr == KERNEL_DIRECT_MAP_BASE + pgd.x,
	       "pgd_to_virt_ptr() not correctly obtaining ptr");

	pudaddr_t pud = early_alloc_pud();
	ptr = pud_to_virt_ptr(pud);
	// Fill for later tests.
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		ptr[i] = i + 1;
	}
	assert((uint64_t)ptr == KERNEL_DIRECT_MAP_BASE + pud.x,
	       "pud_to_virt_ptr() not correctly obtaining ptr");

	pmdaddr_t pmd = early_alloc_pmd();
	ptr = pmd_to_virt_ptr(pmd);
	// Fill for later tests.
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		ptr[i] = i + 1;
	}
	assert((uint64_t)ptr == KERNEL_DIRECT_MAP_BASE + pmd.x,
	       "pmd_to_virt_ptr() not correctly obtaining ptr");

	ptdaddr_t ptd = early_alloc_ptd();
	ptr = ptd_to_virt_ptr(ptd);
	// Fill for later tests.
	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		ptr[i] = i + 1;
	}
	assert((uint64_t)ptr == KERNEL_DIRECT_MAP_BASE + ptd.x,
	       "ptd_to_virt_ptr() not correctly obtaining ptr");

	// Allocate data page for later tests.
	physaddr_t data = early_page_alloc();

	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		pgde_t *pgde_ptr = pgde_at(pgd, i);
		assert(pgde_ptr->x == (uint64_t)(i + 1),
		       "pgde_at() not retrieving correct PGDE");
		uint64_t *ptr = pgd_to_virt_ptr(pgd);
		pgde_ptr->x = NUM_PAGE_TABLE_ENTRIES - i;
		assert((int)ptr[i] == NUM_PAGE_TABLE_ENTRIES - i,
		       "Unable to assign from pgd_at()");

		pude_t *pude_ptr = pude_at(pud, i);
		assert(pude_ptr->x == (uint64_t)(i + 1),
		       "pude_at() not retrieving correct PUDE");
		ptr = pud_to_virt_ptr(pud);
		pude_ptr->x = NUM_PAGE_TABLE_ENTRIES - i;
		assert((int)ptr[i] == NUM_PAGE_TABLE_ENTRIES - i,
		       "Unable to assign from pud_at()");

		pmde_t *pmde_ptr = pmde_at(pmd, i);
		assert(pmde_ptr->x == (uint64_t)(i + 1),
		       "pmde_at() not retrieving correct PMDE");
		ptr = pmd_to_virt_ptr(pmd);
		pmde_ptr->x = NUM_PAGE_TABLE_ENTRIES - i;
		assert((int)ptr[i] == NUM_PAGE_TABLE_ENTRIES - i,
		       "Unable to assign from pmd_at()");

		ptde_t *ptde_ptr = ptde_at(ptd, i);
		assert(ptde_ptr->x == (uint64_t)(i + 1),
		       "ptde_at() not retrieving correct PTDE");
		ptr = ptd_to_virt_ptr(ptd);
		ptde_ptr->x = NUM_PAGE_TABLE_ENTRIES - i;
		assert((int)ptr[i] == NUM_PAGE_TABLE_ENTRIES - i,
		       "Unable to assign from ptd_at()");
	}

	for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		assign_pud(pgd, i, pud);
		pgde_t pgde = *pgde_at(pgd, i);
		assert(pgde.x == (pud.x | PAGE_FLAG_DEFAULT),
		       "Cannot assign PUD to PGD");
		assert(pgde_pud(pgde).x == pud.x,
		       "pgde_pud() not retrieving PUD address");

		assign_pmd(pud, i, pmd);
		pude_t pude = *pude_at(pud, i);
		assert(pude.x == (pmd.x | PAGE_FLAG_DEFAULT),
		       "Cannot assign PMD to PUD");
		assert(pude_pmd(pude).x == pmd.x,
		       "pude_pmd() not retrieving PMD address");

		assign_ptd(pmd, i, ptd);
		pmde_t pmde = *pmde_at(pmd, i);
		assert(pmde.x == (ptd.x | PAGE_FLAG_DEFAULT),
		       "Cannot assign PTD to PMD");
		assert(pmde_ptd(pmde).x == ptd.x,
		       "pmde_ptd() not retrieving PTD address");

		assign_data(ptd, i, data, MAP_KERNEL_NOGLOBAL);
		ptde_t ptde = *ptde_at(ptd, i);
		assert(ptde.x == (data.x | PAGE_FLAG_DEFAULT | PAGE_FLAG_NX),
		       "Cannot assign data page to PTD");
		assert(ptde_data(ptde).x == data.x,
		       "ptde_data() not retrieving data page address");

		assign_data_1gib(pud, i, data, MAP_KERNEL_NOGLOBAL);
		pude = *pude_at(pud, i);
		assert(pude.x ==
			       ((data.x & ~PAGE_MASK_1GIB) | PAGE_FLAG_DEFAULT |
				PAGE_FLAG_NX | PAGE_FLAG_PSE),
		       "Cannot assign 1 GiB data page to PUD");
		assert(pude_data_1gib(pude).x == (data.x & ~PAGE_MASK_1GIB),
		       "pude_data_1gib() not retrieving data page address");

		assign_data_2mib(pmd, i, data, MAP_KERNEL_NOGLOBAL);
		pmde = *pmde_at(pmd, i);
		assert(pmde.x ==
			       ((data.x & ~PAGE_MASK_2MIB) | PAGE_FLAG_DEFAULT |
				PAGE_FLAG_NX | PAGE_FLAG_PSE),
		       "Cannot assign 2 MiB data page to PMD");
		assert(pmde_data_2mib(pmde).x == (data.x & ~PAGE_MASK_2MIB),
		       "pmde_data_2mib() not retrieving data page address");
	}

	// Create a noisy entry and assert we can still get correct data address
	// from it.
	pgde.x = 0xdeadb000 | BIT_MASK_BELOW(PAGE_SHIFT);
	pgde.x |= BIT_MASK(63) | BIT_MASK(50);
	assert(pgde_pud(pgde).x == 0xdeadb000, "Noisy pgde_pud() failed");

	pude.x = 0xdeadb000 | BIT_MASK_BELOW(PAGE_SHIFT);
	pude.x |= BIT_MASK(63) | BIT_MASK(50);
	assert(pude_pmd(pude).x == 0xdeadb000, "Noisy pude_pmd() failed");

	pmde.x = 0xdeadb000 | BIT_MASK_BELOW(PAGE_SHIFT);
	pmde.x |= BIT_MASK(63) | BIT_MASK(50);
	assert(pmde_ptd(pmde).x == 0xdeadb000, "Noisy pmde_ptd() failed");

	ptde.x = 0xdeadb000 | BIT_MASK_BELOW(PAGE_SHIFT);
	ptde.x |= BIT_MASK(63) | BIT_MASK(50);
	assert(ptde_data(ptde).x == 0xdeadb000, "Noisy ptde_data() failed");

	// Now try some larger page size entries.
	pude.x = 0x124fffffff;
	pude.x |= BIT_MASK(63) | BIT_MASK(50);
	assert(pude_data_1gib(pude).x == 0x1240000000,
	       "Noisy pude_data_1gib() failed");
	pmde.x = 0x4fffff;
	pmde.x |= BIT_MASK(63) | BIT_MASK(50);
	assert(pmde_data_2mib(pmde).x == 0x400000,
	       "Noisy pmde_data_2mib() failed");

	early_free_pgd(pgd);
	early_free_pud(pud);
	early_free_pmd(pmd);
	early_free_ptd(ptd);
	early_page_free(data);

	return NULL;
}

static const char *assert_memory_map_basic(void)
{
	pgdaddr_t pgd = early_alloc_pgd();

	struct page_allocators alloc = {
		.pud = early_alloc_pud,
		.pmd = early_alloc_pmd,
		.ptd = early_alloc_ptd,

		.panic = _early_panic,
	};

	// Map a single 4 KiB page.

	virtaddr_t va = {0};
	physaddr_t pa = {0x3000};

	uint64_t num_alloc =
		_map_page_range(pgd, va, pa, 1, MAP_KERNEL_NOGLOBAL, &alloc);

	assert(num_alloc == 3,
	       "Didn't allocate 3 page level tables for 1 page alloc");

	pgde_t pgde = *pgde_at(pgd, 0);
	assert(pgde_present(pgde), "PGDE not present?");

	pudaddr_t pud = pgde_pud(pgde);
	pude_t pude = *pude_at(pud, 0);
	assert(pude_present(pude), "PUDE not present?");

	pmdaddr_t pmd = pude_pmd(pude);
	pmde_t pmde = *pmde_at(pmd, 0);
	assert(pmde_present(pmde), "PMDE not present?");

	ptdaddr_t ptd = pmde_ptd(pmde);
	ptde_t ptde = *ptde_at(ptd, 0);
	assert(ptde_present(ptde), "PTDE not present?");

	assert(IS_SET(ptde.x, PAGE_FLAG_NX_BIT), "NX bit not set?");
	assert(IS_SET(ptde.x, PAGE_FLAG_RW_BIT), "RW bit not set?");

	assert(ptde_data(ptde).x == pa.x, "PA not assigned to PTDE?");

	// Map pages for the remainder of the PTD. We shouldn't be allocating
	// any new pages.

	va = encode_virt(0, 0, 0, 1, 0);
	pa.x = 0x4000;

	num_alloc += _map_page_range(pgd, va, pa, NUM_PAGES_PTD - 1,
				     MAP_KERNEL_NOGLOBAL, &alloc);

	pudaddr_t next_pud = pgde_pud(*pgde_at(pgd, 0));
	pmdaddr_t next_pmd = pude_pmd(*pude_at(pud, 0));
	ptdaddr_t next_ptd = pmde_ptd(*pmde_at(pmd, 0));
	assert(next_pud.x == pud.x, "PUD changed?");
	assert(next_pmd.x == pmd.x, "PMD changed?");
	assert(next_ptd.x == ptd.x, "PTD changed?");

	assert(num_alloc == 3,
	       "Allocated new page tables for entries in existing PTD?");

	for (int i = 1; i < NUM_PAGES_PTD; i++) {
		ptde = *ptde_at(ptd, i);
		assert(ptde_present(ptde), "PTDE not present?");
		assert(IS_SET(ptde.x, PAGE_FLAG_NX_BIT), "NX bit not set?");
		assert(IS_SET(ptde.x, PAGE_FLAG_RW_BIT), "RW bit not set?");
		assert(ptde_data(ptde).x == pa.x, "PA not mapped to PTDE");
		pa = phys_offset_pages(pa, 1);
		va = virt_offset_pages(va, 1);
	}

	// Map a new PTD.

	va = encode_virt(0, 0, 1, 0, 0);

	num_alloc +=
		_map_page_range(pgd, va, pa, 1, MAP_KERNEL_NOGLOBAL, &alloc);

	assert(num_alloc == 4, "New PTD not allocated?");

	pgde = *pgde_at(pgd, 0);
	next_pud = pgde_pud(pgde);
	assert(next_pud.x == pud.x, "PUD changed?");

	pude = *pude_at(pud, 0);
	next_pmd = pude_pmd(pude);
	assert(next_pmd.x == pmd.x, "PMD changed?");

	pmde = *pmde_at(pmd, 1);
	assert(pmde_present(pmde), "PMDE not present?");
	next_ptd = pmde_ptd(pmde);
	assert(next_ptd.x != ptd.x, "PTD the same?");
	ptd = next_ptd;

	ptde = *ptde_at(next_ptd, 0);
	assert(ptde_present(ptde), "PTDE not present?");
	assert(ptde_data(ptde).x == pa.x, "PA not assigned to PTDE?");

	// Map a new PMD.

	va = encode_virt(0, 1, 0, 0, 0);
	pa.x = 0xdeadbe000;

	num_alloc +=
		_map_page_range(pgd, va, pa, 1, MAP_KERNEL_NOGLOBAL, &alloc);

	assert(num_alloc == 6, "New PMD, PTD not allocated?");

	pgde = *pgde_at(pgd, 0);
	next_pud = pgde_pud(pgde);
	assert(next_pud.x == pud.x, "PUD changed?");

	pude = *pude_at(pud, 1);
	next_pmd = pude_pmd(pude);
	assert(next_pmd.x != pmd.x, "PMD the same?");
	pmd = next_pmd;

	pmde = *pmde_at(next_pmd, 0);
	assert(pmde_present(pmde), "PMDE not present?");
	next_ptd = pmde_ptd(pmde);
	assert(next_ptd.x != ptd.x, "PTD the same?");
	ptd = next_ptd;

	ptde = *ptde_at(next_ptd, 0);
	assert(ptde_present(ptde), "PTDE not present?");
	assert(ptde_data(ptde).x == pa.x, "PA not assigned to PTDE?");

	// Map a new PUD.

	va = encode_virt(1, 0, 0, 0, 0);
	pa.x = 0xf00b000;

	num_alloc +=
		_map_page_range(pgd, va, pa, 1, MAP_KERNEL_NOGLOBAL, &alloc);

	assert(num_alloc == 9, "New PUD, PMD, PTD not allocated?");

	pgde = *pgde_at(pgd, 1);
	next_pud = pgde_pud(pgde);
	assert(next_pud.x != pud.x, "PUD the same?");
	pud = next_pud;

	pude = *pude_at(pud, 0);
	assert(pude_present(pude), "PUDE not present?");
	next_pmd = pude_pmd(pude);
	assert(next_pmd.x != pmd.x, "PMD the same?");
	pmd = next_pmd;

	pmde = *pmde_at(next_pmd, 0);
	assert(pmde_present(pmde), "PMDE not present?");
	next_ptd = pmde_ptd(pmde);
	assert(next_ptd.x != ptd.x, "PTD the same?");
	ptd = next_ptd;

	ptde = *ptde_at(next_ptd, 0);
	assert(ptde_present(ptde), "PTDE not present?");
	assert(ptde_data(ptde).x == pa.x, "PA not assigned to PTDE?");

	// We have only allocated a trivial amount of memory so we'll leak the
	// pages.

	return NULL;
}

static const char *assert_memory_map_ranges(void)
{
	return NULL;
}

const char *test_page(void)
{
	const char *ret = assert_correct_virtaddr();
	if (ret != NULL)
		return ret;

	ret = assert_page_misc();
	if (ret != NULL)
		return ret;

	ret = assert_pagetable_helpers();
	if (ret != NULL)
		return ret;

	ret = assert_memory_map_basic();
	if (ret != NULL)
		return ret;

	ret = assert_memory_map_ranges();
	if (ret != NULL)
		return ret;

	return NULL;
}
