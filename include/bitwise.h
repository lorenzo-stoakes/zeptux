#pragma once

#include "compiler.h"
#include "types.h"

// Find the offset of the first series of `n` or more set bits in `x` or -1 if
// we cannot find it (counting from little end).
//
// Adapted, with love, from figure 6-5 in the ever delightful Hacker's Delight
// 2nd edition by H. S. Warren Jr, page 125.
static inline int find_bitstring_longer_than(uint64_t x, int n)
{
	int s;

	while (n > 1) {
		s = n >> 1;
		x &= x << s;
		n -= s;
	}

	return find_first_set_bit(x);
}
