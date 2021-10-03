#include "zeptux_early.h"

void panic(const char *fmt, ...)
{
	enum kernel_stage stage = global_get_stage();
	switch (stage) {
	case KERNEL_STAGE_1_EARLY:
	case KERNEL_STAGE_2_HAS_RING_BUFFER:
		va_list list;
		va_start(list, fmt);
		_early_vpanic(fmt, list); // Doesn't return.
		break;
	default:
		early_panic("Panic unsupported for stage %d", stage);
	}
}
