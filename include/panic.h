#pragma once

#include "types.h"

static inline void panic(const char *why)
{
	// Assumes printf is defined.
	printf("panic: %s\n", why);
	while (true)
		;
}
