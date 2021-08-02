#include "mem.h"
#include "io.h"
#include "types.h"

void load()
{
	// Say hello from 64-bit mode :)
	char hello[] = "Hello";
	volatile char *video = (volatile char*)0xb8000;
	for (char* chr = hello; *chr != '\0'; chr++) {
		*video++ = *chr;
		*video++ = 15;
	}
}
