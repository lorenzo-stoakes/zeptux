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

// Set the `dest` buffer to contain `count` bytes of `chr`.
static inline void *memset(void *dest, int chr, uint64_t count)
{
	// A naive implementation, something to get us started.

	char *target = (char *)dest;

	for (uint64_t i = 0; i < count; i++) {
		*target++ = chr;
	}

	return dest;
}

// Copy memory from `src` to `dest` of size `n` bytes. Returns `dest` for
// convenience.
static inline void *memcpy(void *dest, void *src, size_t n)
{
	// A naive implementation, something to get us started.
	uint8_t *ptr_dest = (uint8_t *)dest;
	uint8_t *ptr_src = (uint8_t *)src;

	for (size_t i = 0; i < n; i++) {
		*ptr_dest++ = *ptr_src++;
	}

	return dest;
}
