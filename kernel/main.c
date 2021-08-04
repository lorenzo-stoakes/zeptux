#include "zeptux_early.h"

struct version_tuplet zeptux_ver = { .maj = 0, .min = 0, .rev = 0 };

int main(void)
{
	early_serial_init_poll();
	fixup_bss();

	printf("\nZeptux %x.%x.%x\n", zeptux_ver.maj, zeptux_ver.min, zeptux_ver.rev);
	return 0;
}
