#pragma once

#define assert(_expr, _msg) \
	if (!(_expr))       \
		return __FILE__ ":" STRINGIFY(__LINE__) ": " _msg;

// test_format.c
const char *test_format(void);

// test_string.c
const char *test_string(void);
