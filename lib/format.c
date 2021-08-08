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

// Try to put a character into buffer `state->buf` of size `state->n` bytes,
// which has already had `state->count` characters placed into it. We leave at
// least 1 byte in the buffer for a null terminator character.
static void _vsnprintf_putc(struct vsnprintf_state *state, char chr)
{
	if (state->count < state->n - 1)
		state->buf[state->count++] = chr;
}

// Try to place string `str` into the target buffer.
static void _vsnprintf_puts(struct vsnprintf_state *state, const char *str)
{
	for (const char *ptr = str; *ptr != '\0'; ptr++) {
		_vsnprintf_putc(state, *ptr);
	}
}

// Try to place uint64 `val` into the target buffer in base `base`. This is the
// core integer output formatter - we output uint32 here by casting the value
// and int64/32 by handling the sign then casting to uint and calling here.
static void _vsnprintf_uint64(struct vsnprintf_state *state, uint64_t val,
			      uint64_t base)
{
	if (val == 0) {
		_vsnprintf_putc(state, '0');
		return;
	}

	int i;
	char val_buf[UINT64_MAX_CHARS + 1];
	for (i = UINT64_MAX_CHARS - 1; val > 0; val /= base, i--) {
		val_buf[i] = digits[val % base];
	}
	val_buf[UINT64_MAX_CHARS] = '\0';

	_vsnprintf_puts(state, &val_buf[i + 1]);
}

// Try to place int64 `val` into the target buffer in base `base`.
static void _vsnprintf_int64(struct vsnprintf_state *state, int64_t val,
			     uint64_t base)
{
	uint64_t un;
	if (val < 0) {
		_vsnprintf_putc(state, '-');
		un = -val;
	} else {
		un = val;
	}

	_vsnprintf_uint64(state, un, base);
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
			_vsnprintf_int64(&state, is64 ? val : (int)val, 10);
			break;
		}
		case 'U':
		case 'u':
		{
			uint64_t val = va_arg(ap, uint64_t);
			_vsnprintf_uint64(&state, is64 ? val : (uint)val, 10);
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
			_vsnprintf_uint64(&state, is64 ? val : (uint)val, 16);
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
			// Simply output unrecognised formats.
			_vsnprintf_putc(&state, '%');
			_vsnprintf_putc(&state, chr);
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
