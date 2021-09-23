#pragma once

#include "consts.h"
#include "types.h"

// Align '_n' to power of 2 '_to'.
#define ALIGN(_n, _to) ((_n) & ~((_to)-1))
// Align '_n' to power of 2 '_to', rounding up.
#define ALIGN_UP(_n, _to) ALIGN((_n) + _to - 1, _to)
// Is '_n' aligned to power of 2 '_to'?
#define IS_ALIGNED(_n, _to) (!((_n) & ((_to)-1)))
// Offset from aligned value.
#define ALIGN_OFFSET(_n, _to) ((_n)-ALIGN((_n), (_to)))

// Determine the number of elements in a C array.
#define ARRAY_COUNT(_arr) (sizeof(_arr) / sizeof(_arr[0]))

// Generate a string literal for the input parameter. We have to do this twice
// due to limitations of the C macro system.
#define STRINGIFY(_x) STRINGIFY2(_x)
#define STRINGIFY2(_x) #_x

// Define the mask representing the specified bit.
#define BIT_MASK(_bit) (1UL << (_bit))
// Define the mask representing everything BUT the specified bit.
#define BIT_MASK_EXCLUDING(_bit) (~BIT_MASK(_bit))
// Determine if the specified bit mask is set in a value.
#define IS_MASK_SET(_val, _mask) (((_val) & (_mask)) == (_mask))
// Determine if the specified bit is set in a value.
#define IS_BIT_SET(_val, _bit) (IS_MASK_SET(_val, BIT_MASK(_bit)))
// Obtain a mask for the bits below that specified, e.g. BIT_MASK_BELOW(3) = 0b111.
#define BIT_MASK_BELOW(_bit) (BIT_MASK(_bit) - 1)
// Obtain a mask for the bits above and including that specified,
// e.g. BIT_MASK_ABOVE(3) = 0b1*000.
#define BIT_MASK_ABOVE(_bit) (~BIT_MASK_BELOW(_bit))

// Wrap a type in a struct to establish some kind of rudimentary C type safety.
#define TYPE_WRAP(_name, _type) \
	typedef struct {        \
		_type x;        \
	} _name

// Suppresses unused parameter warnings.
#define IGNORE_PARAM(_param) (void)_param
