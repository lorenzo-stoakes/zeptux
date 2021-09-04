#pragma once

#include "types.h"

// Compare 2 strings returning 0 if they are equal, a positive value if the
// first differing character is greater in string `a` than `b` or negative if
// vice-versa.
static inline int strcmp(const char *a, const char *b)
{
	for (; *a != '\0' && *b != '\0'; a++, b++) {
		if (*a != *b)
			break;
	}

	return *a - *b;
}

// Determine the length of a null-terminated string.
static inline size_t strlen(const char *str)
{
	uint64_t len = 0;

	while (*str++ != '\0')
		len++;

	return len;
}
