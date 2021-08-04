#pragma once

#include "consts.h"
#include "early_serial.h"
#include "format.h"
#include "types.h"

#define printf(_fmt, ...) \
	printf_to_putc(early_serial_putc, (_fmt) __VA_OPT__(,) __VA_ARGS__)

static inline void panic(const char *why)
{
	printf("panic: %s\n", why);
	while (true)
		;
}
