#pragma once

#include "consts.h"
#include "types.h"

// Align '_n' to power of 2 '_to'.
#define ALIGN(_n, _to) ((_n) & ~((_to)-1))

// Align '_n' to power of 2 '_to', rounding up.
#define ALIGN_UP(_n, _to) ALIGN((_n) + _to - 1, _to)

// Is '_n' aligned to power of 2 '_to'?
#define IS_ALIGNED(_n, _to) (!((_n) & ((_to)-1)))

// Determine the number of elements in a C array.
#define ARRAY_COUNT(_arr) (sizeof(_arr) / sizeof(_arr[0]))

// Generate a string literal for the input parameter. We have to do this twice
// due to limitations of the C macro system.
#define STRINGIFY(_x) STRINGIFY2(_x)
#define STRINGIFY2(_x) #_x

// Helpful bitmask macros.
#define BIT_MASK(_bit) (1UL << _bit)
#define IS_SET(_val, _bit) !!((_val & BIT_MASK(_bit)) == BIT_MASK(_bit))

// Wrap a type in a struct to establish some kind of rudimentary C type safety.
#define TYPE_WRAP(_name, _type) \
	typedef struct {        \
		_type x;        \
	} _name
