#pragma once

#include "types.h"

static inline int strcmp(const char *a, const char *b)
{
	while (*a != '\0' && *b != '\0' && *a++ == *b++)
		;

	return *a - *b;
}
