#pragma once

#include "asm.h"
#include "atomic.h"
#include "compiler.h"
#include "types.h"

// Implementation heavily inspired by https://rigtorp.se/spinlock/

// Represents a spinlock object. Use a struct wrapper as a cheap form of C type
// safety.
typedef struct {
	atomic_t x;
} spinlock_t;

// Simple mechanism for assigning an empty unlocked spinlock.
static inline spinlock_t empty_spinlock(void)
{
	return (spinlock_t){0};
}

static inline void spinlock_acquire(spinlock_t *lock)
{
	while (true) {
		// Attempt to acquire the lock right away. If the lock is
		// available from the start we acquire it right away.
		if (!atomic_exchange_acquire(&lock->x, 1))
			return;

		// Loop on _reads_ of the atomic value. Cache lines can be
		// shared between cores on read, a write requires an update via
		// the cache coherent mechanism.
		while (atomic_load_relaxed(&lock->x)) {
			hint_spinwait();
		}
	}
}

static inline void spinlock_release(spinlock_t *lock)
{
	atomic_store_release(&lock->x, 0);
}
