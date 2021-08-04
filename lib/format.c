#include "compiler.h"
#include "format.h"
#include "macros.h"
#include "consts.h"
#include "types.h"

static const char *digits = "0123456789abcdef";

static void putc_str(putc_fn putc, const char *str)
{
	for (const char *ptr = str; *ptr != '\0'; ptr++) {
		char chr = *ptr;
		putc(chr);
	}
}

static void putc_uint64(putc_fn putc, uint64_t n, uint64_t base)
{
	if (n == 0) {
		putc('0');
		return;
	}

	char buf[UINT64_MAX_CHARS + 1];

	int i;
	for (i = UINT64_MAX_CHARS - 1; n > 0; n /= base, i--) {
		buf[i] = digits[n % base];
	}
	buf[UINT64_MAX_CHARS] = '\0';

	putc_str(putc, &buf[i + 1]);
}

static void putc_int64(putc_fn putc, int64_t n, uint64_t base)
{
	uint64_t un;
	if (n < 0) {
		putc('-');
		un = -n;
	} else {
		un = n;
	}

	putc_uint64(putc, un, base);
}

// Print the format string using the specified putc functino.
void printf_to_putc(putc_fn putc, const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);

	char chr;
	while ((chr = *fmt++) != '\0') {
		if (chr != '%') {
			putc(chr);
			continue;
		}

		chr = *fmt++;
		bool is64 = false;
		if (chr == 'l' || chr == 'L') {
			is64 = true;
			chr = *fmt++;
		}

		switch (chr) {
		case '\0':
			return;
		case 'D':
		case 'd':
		{
			int64_t val = va_arg(list, int64_t);
			putc_int64(putc, is64 ? val : (int)val, 10);
			break;
		}
		case 'U':
		case 'u':
		{
			uint64_t val = va_arg(list, uint64_t);
			putc_uint64(putc, is64 ? val : (uint)val, 10);
			break;
		}
		case 'P':
		case 'p':
			is64 = true;
			// fallthrough
		case 'X':
		case 'x':
		{
			uint64_t val = va_arg(list, uint64_t);
			putc_uint64(putc, is64 ? val : (uint)val, 16);
			break;
		}
		case 'S':
		case 's':
		{
			const char *str = va_arg(list, const char*);
			putc_str(putc, str);
			break;
		}
		case '%':
			putc('%');
			break;
		default:
			// Simply output unrecognised formats.
			putc('%');
			putc(chr);
			break;
		}
	}

	va_end(list);
}
