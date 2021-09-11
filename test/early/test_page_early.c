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

/*
 * TODO:
 * p*d_to_virt_ptr()
 * p*de_present()
 * p*d_index()
 * assign_p*d()
 */

const char *test_page(void)
{
	const char *ret = assert_correct_virtaddr();
	if (ret != NULL)
		return ret;

	ret = assert_page_misc();
	if (ret != NULL)
		return ret;

	return NULL;
}
