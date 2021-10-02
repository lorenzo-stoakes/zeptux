#include "zeptux.h"

static struct kernel_log_state *state;

void kernel_log_init(void)
{
	uint64_t bytes =
		sizeof(struct kernel_log_state) +
		KERNEL_LOG_MAX_NUM_ENTRIES * sizeof(struct kernel_log_entry);
	state = kzalloc(bytes, KMALLOC_KERNEL);
}
