#include "zeptux_early.h"

struct version_tuplet zeptux_ver = {.maj = 0, .min = 0, .rev = 0};

// Provide some early kernel information.
static void prelude(void)
{
	log_info("-- zeptux %u.%u.%u --", zeptux_ver.maj, zeptux_ver.min,
		 zeptux_ver.rev);
	log_info("");
	log_info("kernel ELF size = %lu",
		 early_get_boot_info()->kernel_elf_size_bytes);

	struct early_boot_info *info = early_get_boot_info();

	// Dump raw E820 memory structure.

	uint64_t count = info->num_e820_entries;
	log_info("e820 entries (%lu):", count);

	uint64_t total_size = 0;
	for (uint64_t i = 0; i < count; i++) {
		struct e820_entry *entry = &info->e820_entries[i];
		uint64_t from = entry->base;
		uint64_t size = entry->size;
		uint64_t to = from + size;

		total_size += size;

		char buf[10];
		log_info("%8s %016lx - %016lx : %s",
			 e820_type_to_string(entry->type), from, to,
			 bytes_to_human(size, buf, ARRAY_COUNT(buf)));
	}

	char buf[10];
	log_info("%56s", bytes_to_human(total_size, buf, ARRAY_COUNT(buf)));
	log_info("total available RAM: %s (%lu bytes)",
		 bytes_to_human(info->total_avail_ram_bytes, buf, sizeof(buf)),
		 info->total_avail_ram_bytes);
	log_info("");

	struct phys_alloc_state *state = phys_get_alloc_state_lock();
	struct phys_alloc_stats *stats = &state->stats;

	log_info("phys_alloc: total=%lu, pg=%lu, pb=%lu, rest=%lu",
		 stats->num_4k_pages, stats->num_pagetable_pages,
		 stats->num_physblock_pages,
		 stats->num_4k_pages - stats->num_free_4k_pages -
			 stats->num_pagetable_pages - stats->num_physblock_pages);

	char orders_buf[150];
	const char array_start_str[] = "            [ ";
	int offset = snprintf(orders_buf, sizeof(orders_buf), array_start_str);

	for (int i = 0; i < MAX_ORDER; i++) {
		struct phys_alloc_order_stats *order_stats = &stats->order[i];

		offset += snprintf(&orders_buf[offset],
				   sizeof(orders_buf) - offset, "%lu, ",
				   order_stats->num_free_pages);
	}
	log_info("%s%lu ]", orders_buf, stats->order[MAX_ORDER].num_free_pages);
	spinlock_release(&state->lock);
}

void main(void)
{
	early_init();
	phys_alloc_init();
	kernel_log_init();

	prelude();

	// We never exit.
	while (true)
		;
}
