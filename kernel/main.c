#include "zeptux_early.h"

struct version_tuplet zeptux_ver = { .maj = 0, .min = 0, .rev = 0 };

void main(void)
{
	early_serial_init_poll();

	printf("\n-- zeptux %u.%u.%u --\n\n", zeptux_ver.maj, zeptux_ver.min, zeptux_ver.rev);
	printf("kernel ELF size = %lu\n", boot_info()->kernel_elf_size_bytes);

	// We never exit.
	while (true)
		;
}
