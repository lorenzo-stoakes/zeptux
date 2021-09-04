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
// output, with provided variadic argument list. Once the kernel is initialised
// all such output will instead be output via the kernel ring buffer. Returns
// the number of characters that would have been written had they fit in the
// early printf buffer.
static inline PRINTF(1, 0) int early_vprintf(const char *fmt, va_list ap)
{
	char buf[EARLY_PRINTF_BUFFER_SIZE];
	int ret = vsnprintf(buf, EARLY_PRINTF_BUFFER_SIZE, fmt, ap);

	early_serial_puts(buf);
	early_video_puts(buf);

	return ret;
}

// Early kernel logging function, outputting using early serial and early video
// output. Once the kernel is initialised all such output will instead be output
// via the kernel ring buffer. Returns the number of characters that would have
// been written had they fit in the early printf buffer.
static inline PRINTF(1, 2) int early_printf(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	int ret = early_vprintf(fmt, list);
	va_end(list);

	return ret;
}

// Early kernel logging function, outputting a full line.
static inline int early_puts(const char *str)
{
	return early_printf("%s\n", str);
}

#define early_panic(fmt, ...)                      \
	_early_panic("at " __FILE__ ":" STRINGIFY( \
		__LINE__) ": " fmt __VA_OPT__(, ) __VA_ARGS__)

// Early kernel panic function, simply outputs the specified panic reason then
// halts. The full fat version will output additional useful information for
// debugging.
static inline PRINTF(1, 2) void _early_panic(const char *fmt, ...)
{
	va_list list;

	early_printf("PANIC (early) ");

	va_start(list, fmt);
	early_vprintf(fmt, list);
	va_end(list);

	early_printf("\n");

	// Busy-wait halt.
	while (true)
		;
}
