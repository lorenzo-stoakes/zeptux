#pragma once

#include "types.h"

static inline uint8_t inb(uint16_t port)
{
	uint8_t val;
	asm volatile("inb %w1, %b0" : "=a"(val) : "Nd"(port));
	return val;
}
