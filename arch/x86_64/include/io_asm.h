#pragma once

#include "types.h"

static inline uint8_t inb(uint16_t port)
{
	uint8_t val;
	asm volatile("inb %w1, %b0" : "=a"(val) : "Nd"(port));
	return val;
}

static inline void outb(uint16_t port, uint8_t data)
{
  asm volatile("outb %0,%1" : : "a" (data), "d" (port));
}

static inline void insl(void *ptr, uint16_t port, int count)
{
  asm volatile("cld; rep insl" :
               "=D" (ptr), "=c" (count) :
               "d" (port), "0" (ptr), "1" (count) :
               "memory", "cc");
}
