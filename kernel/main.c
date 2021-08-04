#include "zeptux_early.h"

int main(void)
{
	early_serial_init_poll();
	early_serial_putstr("\nHello, world!\n");

	return 0;
}
