#pragma once

#include "compiler.h"
#include "types.h"

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) PRINTF(3, 0);
int snprintf(char *buf, size_t n, const char *fmt, ...) PRINTF(3, 4);
