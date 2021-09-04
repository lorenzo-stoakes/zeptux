#pragma once
#ifdef __ZEPTUX_KERNEL

// Fundamental types. We want to be explicit on signedness and size so use the
// standard syntax of `[u]int[16|32|64]_t` like stdint.h in userland code.
typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long uint64_t;
typedef long int64_t;
typedef _Bool bool;
typedef uint32_t uint;
typedef uint64_t size_t;
typedef int64_t ssize_t;

// TODO: Perhaps should be in compiler-specific header?
typedef __builtin_va_list va_list;

#endif
