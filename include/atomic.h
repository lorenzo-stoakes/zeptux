#pragma once

#include "compiler.h"
#include "macros.h"
#include "types.h"

// Represents an atomic value.
TYPE_WRAP(atomic_t, uint32_t);

// Wrappers around built-in atomic functions. These use the C++ memory model
// concepts e.g. 'relaxed', 'acquire', 'release' etc.
// See https://en.cppreference.com/w/cpp/atomic/memory_order
#define atomic_load_relaxed(_ptr) _atomic_load_relaxed(&(_ptr)->x)
#define atomic_exchange_acquire(_ptr, _val) \
	_atomic_exchange_acquire(&(_ptr)->x, _val)
#define atomic_store_release(_ptr, _val) _atomic_store_release(&(_ptr)->x, _val)
