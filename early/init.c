#include "zeptux_early.h"

void early_init(void)
{
	early_serial_init_poll();
	early_video_init();
}
