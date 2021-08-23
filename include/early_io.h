#pragma once

#include "consts.h"
#include "early_serial.h"
#include "early_video.h"
#include "format.h"
#include "types.h"

// We vsnprintf into a buffer of this size before outputting using
// early_printf().
#define EARLY_PRINTF_BUFFER_SIZE (1024)

// Early kernel logging function, outputting using early serial and early video
// output. Once the kernel is initialised all such output will instead be output
// via the kernel ring buffer.
static inline PRINTF(1, 2) int early_printf(const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	char buf[EARLY_PRINTF_BUFFER_SIZE];
	int ret = vsnprintf(buf, EARLY_PRINTF_BUFFER_SIZE, fmt, list);

	va_end(list);

	early_serial_puts(buf);
	early_video_puts(buf);

	return ret;
}

// Early kernel logging function, outputting a full line.
static inline int early_puts(const char *str)
{
	return early_printf("%s\n", str);
}

// Early kernel panic function, simply outputs the specified panic reason. The
// full fat version will output additional useful information for debugging.
static inline void early_panic(const char *why)
{
	early_printf("panic: %s\n", why);
	while (true)
		;
}
