#pragma once

#define PACKED __attribute__((packed))
#define static_assert _Static_assert

// Since parameters get put in registers we need help from the compiler to
// sanely handle variadic arguments.
#define va_start(_list, _param) __builtin_va_start(_list, _param)
#define va_end(_list) __builtin_va_end(_list)
#define va_arg(_list, _type) __builtin_va_arg(_list, _type)
