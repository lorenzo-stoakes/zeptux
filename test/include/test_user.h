#pragma once

#include <string>

// Duplicated from macros.h but userland tests can't import that so appears
// necessary.
#define STRINGIFY(_x) STRINGIFY2(_x)
#define STRINGIFY2(_x) #_x

#define assert(_expr, _msg)                                    \
	if (!(_expr)) {                                        \
		std::ostringstream _oss;                       \
		_oss << __FILE__ ":" STRINGIFY(__LINE__) ": "; \
		_oss << _msg;                                  \
		return _oss.str();                             \
	}

const std::string test_range();
