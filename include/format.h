#pragma once

#include "compiler.h"
#include "macros.h"
#include "types.h"

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) PRINTF(3, 0);
int snprintf(char *buf, size_t n, const char *fmt, ...) PRINTF(3, 4);

// Outputs a human-readable version of `bytes` in `out_buf`. Returns `buf` for convenience.
char *bytes_to_human(uint64_t bytes, char *buf, uint64_t buf_size);
