#include "test_early.h"

void main(void)
{
	early_init();

	const char *res = test_format();
	if (res != NULL)
		early_puts(res);

	res = test_string();
	if (res != NULL)
		early_puts(res);

	res = test_misc();
	if (res != NULL)
		early_puts(res);

	res = test_mem();
	if (res != NULL)
		early_puts(res);

	res = test_page();
	if (res != NULL)
		early_puts(res);

	// Tests after this point rely on physical memory allocator being
	// initiated.
	phys_alloc_init();

	res = test_phys_alloc();
	if (res != NULL)
		early_puts(res);

	early_puts("// zeptux EARLY test run complete");
	exit_qemu();
}
