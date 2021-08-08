#include "format.h"
#include "compiler.h"
#include "consts.h"
#include "macros.h"
#include "types.h"

static const char *digits = "0123456789abcdef";

// Represents the current state of the vsnprintf operation.
struct vsnprintf_state {
	char *buf;
	size_t n, count;
};

// Represents integer format for a specific integer being output via
// vsnprintf().
struct vsnprintf_integer_format {
	int base;
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
		state->buf[state->count++] = chr;
}

// Place `chr` into the target buffer `n` times.
static void _vsnprintf_putc_repeat(struct vsnprintf_state *state, char chr,
				   int n)
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

// Try to place uint64 `val` into the target buffer in base `format->base`. This
// is the core integer output formatter - we output uint32 here by casting the
// value and int64/32 by handling the sign then casting to uint and calling
// here.
static void _vsnprintf_uint64(struct vsnprintf_state *state, uint64_t val,
			      struct vsnprintf_integer_format *format)
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
			     struct vsnprintf_integer_format *format)
{
	uint64_t un;
	if (val < 0) {
		_vsnprintf_putc(state, '-');
		un = -val;
	} else {
		un = val;
	}

	_vsnprintf_uint64(state, un, format);
}

// Output at most `n` bytes to buffer `buf` (which will be of at most `n - 1` in
// STRING length) using variable list ap for arguments. We append a null
// terminator. Returns the number of characters that would have been written had
// `n` been sufficiently large.
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
		struct vsnprintf_integer_format int_format = {
			.base = 10,
			.pad_char = ' ',
			.pad_count = 0,
			.pad_left = false,
		};

		if (chr == '-') {
			int_format.pad_left = true;
			chr = *fmt++;
		}

		if (chr == '0' || chr == ' ') {
			int_format.pad_char = chr;
			chr = *fmt++;
		}

		while (chr >= '0' && chr <= '9') {
			int_format.pad_count *= 10;
			int_format.pad_count += chr - '0';
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

			_vsnprintf_int64(&state, is64 ? val : (int)val,
					 &int_format);
			break;
		}
		case 'U':
		case 'u':
		{
			uint64_t val = va_arg(ap, uint64_t);

			_vsnprintf_uint64(&state, is64 ? val : (uint)val,
					  &int_format);
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

			int_format.base = 16;
			_vsnprintf_uint64(&state, is64 ? val : (uint)val,
					  &int_format);
			break;
		}
		case 'S':
		case 's':
		{
			const char *str = va_arg(ap, const char *);
			_vsnprintf_puts(&state, str);
			break;
		}
		case '%':
			_vsnprintf_putc(&state, '%');
			break;
		default:
			_vsnprintf_puts(&state, "%format error%");
			break;
		}
	}

out:
	size_t offset = state.count < state.n - 1 ? state.count : state.n - 1;
	buf[offset] = '\0';

	return (int)state.count;
}

// Output at most `n` bytes to buffer `buf` (which will be of at most `n - 1` in
// STRING length). We append a null terminator. Returns the number of characters
// that would have been written had `n` been sufficiently large.
int snprintf(char *buf, size_t n, const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	int ret = vsnprintf(buf, n, fmt, list);
	va_end(list);

	return ret;
}
