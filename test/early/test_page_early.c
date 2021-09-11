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
	       "virt_to_phys() not decoded ELF VA well");

	ptr = (void *)va.x;
	assert(virt_ptr_to_phys(ptr).x == 0xbeef,
	       "virt_ptr_to_phys() not decoded ELF VA well");

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
		assert(pgde_at(pgd, i)->x == (pud.x | PAGE_FLAG_DEFAULT),
		       "Cannot assign PUD to PGD");

		assign_pmd(pud, i, pmd);
		assert(pude_at(pud, i)->x == (pmd.x | PAGE_FLAG_DEFAULT),
		       "Cannot assign PMD to PUD");

		assign_ptd(pmd, i, ptd);
		assert(pmde_at(pmd, i)->x == (ptd.x | PAGE_FLAG_DEFAULT),
		       "Cannot assign PTD to PMD");
	}

	early_free_pgd(pgd);
	early_free_pud(pud);
	early_free_pmd(pmd);
	early_free_ptd(ptd);

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

	return NULL;
}
