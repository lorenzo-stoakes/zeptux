#include "zeptux_early.h"

struct version_tuplet zeptux_ver = { .maj = 0, .min = 0, .rev = 0 };

static void prelude()
{
	printf("\n-- zeptux %u.%u.%u --\n\n", zeptux_ver.maj, zeptux_ver.min,
	       zeptux_ver.rev);
	printf("kernel ELF size = %lu\n", boot_info()->kernel_elf_size_bytes);

	// Dump raw E820 memory structure.

	uint16_t count = boot_info()->num_e820_entries;
	printf("\ne820 entries (%lu):\n", count);

	for (uint64_t i = 0; i < count; i++) {
		struct e820_entry *entry = &boot_info()->e820_entries[i];


		printf("\t%s\t%lx - %lx\n", e820_type_to_string(entry->type),
		       entry->base, entry->base + entry->size);
	}
}

void main(void)
{
	early_serial_init_poll();
	prelude();

	// We never exit.
	while (true)
		;
}
