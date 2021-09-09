#pragma once

#include <cstdint> // Needed to fake out the stdarg zeptux kernel replacements.
#include <sstream>
#include <string>

// To prevent duplicate definition in zeptux headers and standard imports.
#undef NULL

// Duplicated from macros.h but userland tests can't import that so appears
// necessary.
#define STRINGIFY(_x) STRINGIFY2(_x)
#define STRINGIFY2(_x) #_x

// Assert helper, designed to be run from a function which returns a string.
#define assert(_expr, _msg)                                    \
	if (!(_expr)) {                                        \
		std::ostringstream _oss;                       \
		_oss << __FILE__ ":" STRINGIFY(__LINE__) ": "; \
		_oss << _msg;                                  \
		return _oss.str();                             \
	}

// test_range.cpp
std::string test_range();

// test_spinlock.cpp
std::string test_spinlock();

// test_misc.cpp
std::string test_misc();

// test_bitmap.cpp
std::string test_bitmap();
