#pragma once

#include "compiler.h"
#include "types.h"

// Represents an atomic value. Use a struct wrapper as a cheap form of C type
// safety.
typedef struct {
	uint32_t x;
} atomic_t;

#define atomic_load_relaxed(_ptr) _atomic_load_relaxed(&(_ptr)->x)
#define atomic_exchange_acquire(_ptr, _val) \
	_atomic_exchange_acquire(&(_ptr)->x, _val)
#define atomic_store_release(_ptr, _val) _atomic_store_release(&(_ptr)->x, _val)
