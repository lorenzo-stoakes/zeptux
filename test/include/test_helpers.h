#pragma once

#define assert(_expr, _msg) \
	if (!(_expr))       \
		return __FILE__ ":" STRINGIFY(__LINE__) ": " _msg;
