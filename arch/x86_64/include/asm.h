#pragma once

#include "compiler.h"
#include "page.h"
#include "types.h"

// Input a single byte from the specified port.
static inline uint8_t inb(uint16_t port)
{
	uint8_t val;
	asm volatile("inb %w1, %b0" : "=a"(val) : "Nd"(port));
	return val;
}

// Output a single byte to the specfied port.
static inline void outb(uint16_t port, uint8_t data)
{
	asm volatile("outb %0,%1" : : "a"(data), "d"(port));
}

// Output a single 16-bit 'word' to the specified port.
static inline void outw(uint16_t port, uint16_t data)
{
	asm volatile("outw %0,%1" : : "a"(data), "d"(port));
}

// Read `count` bytes of data from the specified port.
static inline void insl(void *ptr, uint16_t port, int count)
{
	asm volatile("cld; rep insl"
		     : "=D"(ptr), "=c"(count)
		     : "d"(port), "0"(ptr), "1"(count)
		     : "memory", "cc");
}

// Force a global TLB (Translation Lookahead Buffer) flush by reloading the PGD.
// Will NOT clear entries marked with the PAGE_GLOBAL flag.
static inline void global_flush_tlb(void)
{
	memory_fence();
	asm volatile("movq %%cr3, %%rax; movq %%rax, %%cr3"
		     :
		     :
		     : "memory", "rax");
}

// Set PGD for current core to specified physical address.
static inline void set_pgd(pgdaddr_t pgd)
{
	memory_fence();
	asm volatile("movq %0, %%cr3" : : "a"(pgd.x) : "memory");
}
