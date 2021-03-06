#include "format.h"
#include "compiler.h"
#include "consts.h"
#include "macros.h"
#include "string.h"
#include "types.h"

static const char *digits = "0123456789abcdef";

// Represents the current state of the vsnprintf operation.
struct vsnprintf_state {
	char *buf;
	size_t n, count;
};

// Represents format options for vsnprintf().
struct vsnprintf_format {
	int base;
	bool neg;
	char pad_char;
	int pad_count;
	bool pad_left;
};

// Try to put a character into buffer `state->buf` of size `state->n` bytes,
// which has already had `state->count` characters placed into it. We leave at
// least 1 byte in the buffer for a null terminator character.
static void _vsnprintf_putc(struct vsnprintf_state *state, char chr)
{
	if (state->count < state->n - 1)
		state->buf[state->count] = chr;

	state->count++;
}

// Place `chr` into the target buffer `n` times.
static void _vsnprintf_putc_repeat(struct vsnprintf_state *state, char chr, int n)
{
	for (int i = 0; i < n; i++) {
		_vsnprintf_putc(state, chr);
	}
}

// Try to place string `str` into the target buffer.
static void _vsnprintf_puts(struct vsnprintf_state *state, const char *str)
{
	for (const char *ptr = str; *ptr != '\0'; ptr++) {
		_vsnprintf_putc(state, *ptr);
	}
}

// Try to place string `str` into the target buffer, with the specified format
// settings.
static void _vsnprintf_puts_format(struct vsnprintf_state *state, const char *str,
				   struct vsnprintf_format *format)
{
	if (format->pad_left)
		_vsnprintf_puts(state, str);

	size_t len = strlen(str);
	if (format->pad_count > (int)len)
		_vsnprintf_putc_repeat(state, ' ', format->pad_count - len);

	if (!format->pad_left)
		_vsnprintf_puts(state, str);
}

// Try to place uint64 `val` into the target buffer in base `format->base`. This
// is the core integer output formatter - we output uint32 here by casting the
// value and int64/32 by handling the sign then casting to uint and calling
// here.
static void _vsnprintf_uint64(struct vsnprintf_state *state, uint64_t val,
			      struct vsnprintf_format *format)
{
	int i = UINT64_MAX_CHARS - 1;
	char val_buf[UINT64_MAX_CHARS + 1];

	if (val == 0) {
		val_buf[UINT64_MAX_CHARS - 1] = '0';
		i--;
	}

	for (; val > 0; val /= format->base, i--) {
		val_buf[i] = digits[val % format->base];
	}
	val_buf[UINT64_MAX_CHARS] = '\0';

	// An irritating detail - if we pad with spaces we need to put the
	// negation char with the number, e.g. %5d -> ' -123'. However if
	// we pad with zeroes it should go first, e.g. %05d -> '-0123'.
	// We disallow padding left with zeroes for obvious reasons.
	if (format->neg && format->pad_char == '0') {
		_vsnprintf_putc(state, '-');
		format->pad_count--;
	} else if (format->neg) {
		val_buf[i--] = '-';
	}

	if (format->pad_left)
		_vsnprintf_puts(state, &val_buf[i + 1]);

	int chars_output = UINT64_MAX_CHARS - i - 1;
	if (format->pad_count > chars_output)
		_vsnprintf_putc_repeat(state, format->pad_char,
				       format->pad_count - chars_output);

	if (!format->pad_left)
		_vsnprintf_puts(state, &val_buf[i + 1]);
}

// Try to place int64 `val` into the target buffer in base `format->base`.
static void _vsnprintf_int64(struct vsnprintf_state *state, int64_t val,
			     struct vsnprintf_format *format)
{
	uint64_t un;
	if (val < 0) {
		format->neg = true;
		un = -val;
	} else {
		un = val;
	}

	_vsnprintf_uint64(state, un, format);
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
	if (n == 0)
		return 0;

	struct vsnprintf_state state = {
		.buf = buf,
		.n = n,
		.count = 0,
	};

	char chr;
	while ((chr = *fmt++) != '\0') {
		if (chr != '%') {
			_vsnprintf_putc(&state, chr);
			continue;
		}
		chr = *fmt++;

		// We set this even when we are not outputting an integer,
		// however we're not going for incredible performance here so
		// we'll live with it!
		struct vsnprintf_format format = {
			.base = 10,
			.neg = false,
			.pad_char = ' ',
			.pad_count = 0,
			.pad_left = false,
		};

		if (chr == '-') {
			format.pad_left = true;
			chr = *fmt++;
		}

		if (!format.pad_left && (chr == '0' || chr == ' ')) {
			format.pad_char = chr;

			chr = *fmt++;
		}

		while (chr >= '0' && chr <= '9') {
			format.pad_count *= 10;
			format.pad_count += chr - '0';
			chr = *fmt++;
		}

		bool is64 = false;
		if (chr == 'l' || chr == 'L') {
			is64 = true;
			chr = *fmt++;
		}

		switch (chr) {
		case '\0':
			goto out;
		case 'D':
		case 'd':
		{
			int64_t val = va_arg(ap, int64_t);

			_vsnprintf_int64(&state, is64 ? val : (int)val, &format);
			break;
		}
		case 'U':
		case 'u':
		{
			uint64_t val = va_arg(ap, uint64_t);

			_vsnprintf_uint64(&state, is64 ? val : (uint)val,
					  &format);
			break;
		}
		case 'P':
		case 'p':
			is64 = true;
			// fallthrough
		case 'X':
		case 'x':
		{
			uint64_t val = va_arg(ap, uint64_t);

			format.base = 16;
			_vsnprintf_uint64(&state, is64 ? val : (uint)val,
					  &format);
			break;
		}
		case 'S':
		case 's':
		{
			const char *str = va_arg(ap, const char *);
			_vsnprintf_puts_format(&state, str, &format);
			break;
		}
		case '%':
			_vsnprintf_putc(&state, '%');
			break;
		default:
			format.pad_count = 0;
			_vsnprintf_puts(&state, "%format error%");
			break;
		}
	}

out:
	size_t offset = state.count < state.n - 1 ? state.count : state.n - 1;
	buf[offset] = '\0';

	return (int)state.count;
}

int snprintf(char *buf, size_t n, const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	int ret = vsnprintf(buf, n, fmt, list);
	va_end(list);

	return ret;
}

char *bytes_to_human(uint64_t bytes, char *buf, uint64_t buf_size)
{
	static const char *units[] = {"bytes", "KiB", "MiB", "GiB", "TiB"};

	uint64_t unit_index = 0;
	uint64_t unit_mult = 1;

	uint64_t num_units = bytes;
	for (uint64_t i = 0; i < ARRAY_COUNT(units);
	     i++, num_units /= 1024, unit_mult *= 1024) {
		if (num_units < 1024) {
			unit_index = i;
			break;
		}
	}

	// If unit is < GiB it's ok to just round off.
	if (unit_index < 3 || bytes % unit_mult == 0) {
		snprintf(buf, buf_size, "%5lu %s", num_units, units[unit_index]);
		return buf;
	}

	// We'll add a single decimal place. As we don't have floating point we
	// have to use integer maths.

	uint64_t remaining = bytes - num_units * unit_mult;
	// Obtain in terms of unit below.
	remaining *= 1024;
	remaining /= unit_mult;
	// Single digit after decimal place.
	uint64_t fract = remaining >= 1000 ? 9 : remaining / 100;

	snprintf(buf, buf_size, "%3lu.%lu %s", num_units, fract,
		 units[unit_index]);
	return buf;
}
