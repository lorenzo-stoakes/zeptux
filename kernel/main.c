#include "zeptux_early.h"

struct version_tuplet zeptux_ver = {.maj = 0, .min = 0, .rev = 0};

// Provide some early kernel information.
static void prelude(void)
{
	early_printf("-- zeptux %u.%u.%u --\n", zeptux_ver.maj, zeptux_ver.min,
		     zeptux_ver.rev);
	early_printf("kernel ELF size = %lu\n",
		     early_get_boot_info()->kernel_elf_size_bytes);

	struct early_boot_info *info = early_get_boot_info();

	// Dump raw E820 memory structure.

	uint64_t count = info->num_e820_entries;
	early_printf("\ne820 entries (%lu):\n", count);

	uint64_t total_size = 0;
	for (uint64_t i = 0; i < count; i++) {
		struct e820_entry *entry = &info->e820_entries[i];
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
		     bytes_to_human(info->total_avail_ram_bytes, buf,
				    sizeof(buf)),
		     info->total_avail_ram_bytes);

	struct phys_alloc_state *state = phys_get_alloc_state_locked();
	struct phys_alloc_stats *stats = &state->stats;

	early_printf("\nphys_alloc: total=%lu, pg=%lu, pb=%lu, rest=%lu\n",
		     stats->num_4k_pages, stats->num_pagetable_pages,
		     stats->num_physblock_pages,
		     stats->num_4k_pages - stats->num_free_4k_pages -
			     stats->num_pagetable_pages -
			     stats->num_physblock_pages);

	early_printf("            [ ");
	for (int i = 0; i < MAX_ORDER; i++) {
		struct phys_alloc_order_stats *order_stats = &stats->order[i];

		early_printf("%lu, ", order_stats->num_free_pages);
	}
	early_printf("%lu ]\n", stats->order[MAX_ORDER].num_free_pages);
	spinlock_release(&state->lock);
}

void main(void)
{
	early_init();
	phys_alloc_init();

	prelude();

	// We never exit.
	while (true)
		;
}
