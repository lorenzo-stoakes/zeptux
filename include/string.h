#pragma once

#include "types.h"

static inline int strcmp(const char *a, const char *b)
{
	for (; *a != '\0' && *b != '\0'; a++, b++) {
		if (*a != *b)
			break;
	}

	return *a - *b;
}

static inline size_t strlen(const char *str)
{
	uint64_t len = 0;

	while (*str++ != '\0')
		len++;

	return len;
}
