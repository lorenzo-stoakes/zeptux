#include "zeptux.h"

static struct kernel_global *global;

void global_init(void *ptr)
{
	global = (struct kernel_global *)ptr;
	global->stage = KERNEL_STAGE_1_EARLY;
	global->log_echo = true; // Default to true.
	global->log_level = KERNEL_LOG_DEBUG;
}

struct kernel_global *global_get_locked(void)
{
	spinlock_acquire(&global->lock);
	return global;
}
