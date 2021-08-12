#include "zeptux_early.h"

struct version_tuplet zeptux_ver = {.maj = 0, .min = 0, .rev = 0};

// Provide some early kernel information.
static void prelude()
{
	early_printf("\n-- zeptux %u.%u.%u --\n", zeptux_ver.maj,
		     zeptux_ver.min, zeptux_ver.rev);
	early_printf("kernel ELF size = %lu\n",
		     boot_info()->kernel_elf_size_bytes);

	// Dump raw E820 memory structure.

	uint16_t count = boot_info()->num_e820_entries;
	early_printf("\ne820 entries (%u):\n", count);

	uint64_t total_size = 0;
	for (uint64_t i = 0; i < count; i++) {
		struct e820_entry *entry = &boot_info()->e820_entries[i];
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
}

void main(void)
{
	early_serial_init_poll();
	early_video_init();

	prelude();

	// We never exit.
	while (true)
		;
}
