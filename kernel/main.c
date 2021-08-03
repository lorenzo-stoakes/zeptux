#include "serial.h"

int main(void)
{
	serial_init_poll();
	serial_putstr("\nHello, world!\n");

	return 0;
}
