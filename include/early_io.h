#pragma once

#include "consts.h"
#include "early_serial.h"
#include "format.h"
#include "types.h"

#define EARLY_PRINTF_BUFFER_SIZE (1024)

static inline PRINTF(1, 2) int early_printf(const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	char buf[EARLY_PRINTF_BUFFER_SIZE];
	int ret = vsnprintf(buf, EARLY_PRINTF_BUFFER_SIZE, fmt, list);

	va_end(list);

	early_serial_puts(buf);

	return ret;
}

static inline void panic(const char *why)
{
	early_printf("panic: %s\n", why);
	while (true)
		;
}
