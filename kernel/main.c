#include "zeptux_early.h"

struct version_tuplet zeptux_ver = {.maj = 0, .min = 0, .rev = 0};

// Provide some early kernel information.
static void prelude(uint64_t total_ram_bytes)
{
	early_printf("-- zeptux %u.%u.%u --\n", zeptux_ver.maj, zeptux_ver.min,
		     zeptux_ver.rev);
	early_printf("kernel ELF size = %lu\n",
		     early_get_boot_info()->kernel_elf_size_bytes);

	// Dump raw E820 memory structure.

	uint16_t count = early_get_boot_info()->num_e820_entries;
	early_printf("\ne820 entries (%u):\n", count);

	uint64_t total_size = 0;
	for (uint64_t i = 0; i < count; i++) {
		struct e820_entry *entry =
			&early_get_boot_info()->e820_entries[i];
		uint64_t from = entry->base;
		uint64_t size = entry->size;
		uint64_t to = from + size;

		total_size += size;

		char buf[10];
		early_printf("%8s %016lx - %016lx : %s\n",
			     e820_type_to_string(entry->type), from, to,
			     bytes_to_human(size, buf, ARRAY_COUNT(buf)));
	}

	char buf[10];
	early_printf("\n%56s\n",
		     bytes_to_human(total_size, buf, ARRAY_COUNT(buf)));
	early_printf("\ntotal available RAM: %s (%lu bytes)\n",
		     bytes_to_human(total_ram_bytes, buf, sizeof(buf)),
		     total_ram_bytes);
}

void main(void)
{
	early_init();
	uint64_t total_ram_bytes = early_meminit();

	prelude(total_ram_bytes);

	// We never exit.
	while (true)
		;
}
