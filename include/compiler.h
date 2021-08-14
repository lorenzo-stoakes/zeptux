#pragma once

// Tell the compiler that we don't want struct padding.
#define PACKED __attribute__((packed))
// Tell the compiler that this function is of the printf-ilk and ask that
// arguments are checked accordingly.
#define PRINTF(__string_idx, __first_check_idx) \
	__attribute__((format(printf, (__string_idx), (__first_check_idx))))
#define static_assert _Static_assert

// Since parameters get put in registers we need help from the compiler to
// sanely handle variadic arguments.
#define va_start(_list, _param) __builtin_va_start(_list, _param)
#define va_end(_list) __builtin_va_end(_list)
#define va_arg(_list, _type) __builtin_va_arg(_list, _type)

// Wrappers around atomic functions.
#define _atomic_load_relaxed(_ptr) __atomic_load_n(_ptr, __ATOMIC_RELAXED)
#define _atomic_exchange_acquire(_ptr, _val) \
	__atomic_exchange_n(_ptr, _val, __ATOMIC_ACQUIRE)
#define _atomic_store_release(_ptr, _val) \
	__atomic_store_n(_ptr, _val, __ATOMIC_RELEASE)
