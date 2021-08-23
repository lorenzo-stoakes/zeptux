#pragma once

#include "compiler.h"
#include "macros.h"
#include "types.h"

// Output at most `n` bytes to buffer `buf` (which will be of at most `n - 1` in
// STRING length) using variable list ap for arguments. We append a null
// terminator. Returns the number of characters that would have been written had
// `n` been sufficiently large.
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) PRINTF(3, 0);

// Output at most `n` bytes to buffer `buf` (which will be of at most `n - 1` in
// STRING length). We append a null terminator. Returns the number of characters
// that would have been written had `n` been sufficiently large.
int snprintf(char *buf, size_t n, const char *fmt, ...) PRINTF(3, 4);

// Outputs a human-readable version of `bytes` in `buf`. Returns `buf` pointer
// for convenience.
char *bytes_to_human(uint64_t bytes, char *buf, uint64_t buf_size);
