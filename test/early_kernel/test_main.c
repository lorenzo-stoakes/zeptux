#include "test.h"
#include "zeptux_early.h"

void main(void)
{
	early_serial_init_poll();
	early_printf("\n--zeptux tests--\n");

	test_format();
}
