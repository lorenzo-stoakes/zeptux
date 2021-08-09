#include "test.h"
#include "zeptux_early.h"

void main(void)
{
	early_serial_init_poll();
	// QEMU doesn't start us on a clear line.
	early_printf("\n");

	const char *res = test_format();
	if (res != NULL)
		early_puts(res);

	res = test_string();
	if (res != NULL)
		early_puts(res);

	res = test_misc();
	if (res != NULL)
		early_puts(res);

	early_puts("\n// zeptux test run complete");

	// We never exit.
	while (true)
		;
}
