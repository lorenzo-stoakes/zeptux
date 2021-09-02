#include "test_early.h"

const char *test_string(void)
{
	assert(strcmp("foo", "fob") > 0, "strcmp(foo, fob) failed");
	assert(strcmp("fob", "foo") < 0, "strcmp(fob, foo) failed");
	assert(strcmp("foo", "foo") == 0, "strcmp(foo, foo) failed");
	assert(strcmp("", "") == 0, "strcmp(,) failed");
	assert(strcmp("foo", "") > 0, "strcmp(foo,) failed");
	assert(strcmp("", "foo") < 0, "strcmp(,foo) failed");
	assert(strcmp("hello", "hi") < 0, "strcmp(hello,hi) failed");
	assert(strcmp("hi", "hello") > 0, "strcmp(hi,hello) failed");

	assert(strlen("") == 0, "strlen() failed");
	assert(strlen("hello") == 5, "strlen(hello) failed");

	return NULL;
}
