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

	early_puts("// zeptux test run complete");

	// We never exit.
	while (true)
		;
}
