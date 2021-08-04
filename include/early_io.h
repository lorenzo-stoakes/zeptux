#pragma once

#include "early_serial.h"
#include "format.h"
#include "types.h"

#define early_printf(_fmt, ...) \
	printf_to_putc(early_serial_putc, (_fmt) __VA_OPT__(,) __VA_ARGS__)
