#include "zeptux_early.h"

struct version_tuplet zeptux_ver = { .maj = 0, .min = 0, .rev = 0 };

void main(void)
{
	early_serial_init_poll();
	//fixup_bss();

	printf("\nzeptux v%u.%u.%u\n", zeptux_ver.maj, zeptux_ver.min, zeptux_ver.rev);

	// We never exit.
	while (true)
		;
}
