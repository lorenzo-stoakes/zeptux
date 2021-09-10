#include "test_early.h"

const char *assert_correct_virtaddr(void)
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

	return NULL;
}

/*
 * TODO:
 * bytes_to_pages()
 * phys_to_virt()
 * phys_to_virt_ptr()
 * p*d_to_virt_ptr()
 * virt_to_phys()
 * virt_ptr_to_phys()
 * zero_page()
 * pa_to_pfn()
 * pfn_to_pa()
 * pa_next|prev_page()
 * p*de_present()
 * p*d_index()
 * assign_p*d()
 */

const char *test_page(void)
{
	const char *ret = assert_correct_virtaddr();
	if (ret != NULL)
		return ret;

	return NULL;
}
