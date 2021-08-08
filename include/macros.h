#pragma once

#include "consts.h"
#include "types.h"

// Align '_n' to power of 2 '_to'.
#define ALIGN(_n, _to) ((_n) & ~((_to)-1))

// Align '_n' to power of 2 '_to', rounding up.
#define ALIGN_UP(_n, _to) ALIGN((_n) + _to - 1, _to)

// Determine the number of elements in a C array.
#define ARRAY_COUNT(_arr) (sizeof(_arr) / sizeof(_arr[0]))
