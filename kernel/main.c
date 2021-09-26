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

	struct early_page_alloc_state *alloc_state = early_get_page_alloc_state();
	uint64_t total_pages = alloc_state->total_pages;
	uint64_t alloc_pages = alloc_state->num_allocated_pages;
	uint64_t pagetable_pages = alloc_state->num_pagetable_pages;
	uint64_t physblock_pages = alloc_state->num_physblock_pages;

	early_printf(
		"\nTotal pages = %lu, allocated = %lu (pagetables = %lu, physblock pages = %lu, rest = %lu)\n",
		total_pages, alloc_pages, pagetable_pages, physblock_pages,
		alloc_pages - pagetable_pages - physblock_pages);
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
