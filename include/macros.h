#pragma once

#include "consts.h"
#include "types.h"

// Align '_n' to power of 2 '_to'.
#define ALIGN(_n, _to) \
	((_n) & ~((_to) - 1))

// Align '_n' to power of 2 '_to', rounding up.
#define ALIGN_UP(_n, _to) \
	ALIGN((_n) + _to - 1, _to)
