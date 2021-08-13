#include "test_early.h"

static const char *test_snprintf(void)
{
	char buf[1000];

	// Simple cases:

	int count = snprintf(buf, 1000, "hello");
	assert(count == 5, "incorrect snprintf count");
	assert(strcmp(buf, "hello") == 0, "simple snprintf string mismatch");

	count = snprintf(buf, 1000, "%d", 1234);
	assert(count == 4, "incorrect snprintf int count");
	assert(strcmp(buf, "1234") == 0, "snprintf int string mismatch");

	count = snprintf(buf, 1000, "%x", 1234);
	assert(count == 3, "incorrect snprintf int hex count");
	assert(strcmp(buf, "4d2") == 0, "snprintf int hex string mismatch");

	const char *str = "hello, world!";
	const char *expected_str = "'hello, world!'";
	count = snprintf(buf, 1000, "'%s'", str);
	assert(count == (int)strlen(expected_str),
	       "incorrect snprintf string count");
	assert(strcmp(buf, expected_str) == 0,
	       "snprintf string string mismatch");

	count = snprintf(buf, 1000, "%d", -1234);
	assert(count == 5, "incorrect snprintf neg int count");
	assert(strcmp(buf, "-1234") == 0, "snprintf neg int string mismatch");

	count = snprintf(buf, 1000, "%d", 2147483647);
	assert(count == 10, "incorrect snprintf max int count");
	assert(strcmp(buf, "2147483647") == 0,
	       "snprintf max int string mismatch");

	count = snprintf(buf, 1000, "%d", (int)-2147483648);
	assert(count == 11, "incorrect snprintf min int count");
	assert(strcmp(buf, "-2147483648") == 0,
	       "snprintf min int string mismatch");

#pragma GCC diagnostic ignored "-Wformat"
	count = snprintf(buf, 1000, "%d", 2147483648L);
#pragma GCC diagnostic pop
	assert(count == 11, "incorrect snprintf overflow int count");
	assert(strcmp(buf, "-2147483648") == 0,
	       "snprintf overflow int mismatch");

#pragma GCC diagnostic ignored "-Wformat"
	count = snprintf(buf, 1000, "%d", -2147483649L);
#pragma GCC diagnostic pop
	assert(count == 10, "incorrect snprintf underflow int count");
	assert(strcmp(buf, "2147483647") == 0,
	       "snprintf underflow int mismatch");

	count = snprintf(buf, 1000, "%u", 4294967295U);
	assert(count == 10, "incorrect snprintf max uint count");
	assert(strcmp(buf, "4294967295") == 0, "snprintf max uint mismatch");

	count = snprintf(buf, 1000, "%ld", 9223372036854775807L);
	assert(count == 19, "incorrect snprintf max long int count");
	assert(strcmp(buf, "9223372036854775807") == 0,
	       "snprintf max long int mismatch");

	count = snprintf(buf, 1000, "%ld", -9223372036854775807L - 1);
	assert(count == 20, "incorrect snprintf min long int count");
	assert(strcmp(buf, "-9223372036854775808") == 0,
	       "snprintf min long int mismatch");

	count = snprintf(buf, 1000, "%5d", 123);
	assert(count == 5, "incorrect snprintf padded right, pad count");
	assert(strcmp(buf, "  123") == 0,
	       "snprintf padded right, pad mismatch");

	count = snprintf(buf, 1000, "%05d", 123);
	assert(count == 5, "incorrect snprintf zero padded right, pad count");
	assert(strcmp(buf, "00123") == 0,
	       "snprintf zero padded right, pad mismatch");

	count = snprintf(buf, 1000, "%-5d", 123);
	assert(count == 5, "incorrect snprintf padded left, pad count");
	assert(strcmp(buf, "123  ") == 0, "snprintf padded left, pad mismatch");

#pragma GCC diagnostic ignored "-Wformat"
	count = snprintf(buf, 1000, "%-05d", 123);
#pragma GCC diagnostic pop
	assert(count == 5, "incorrect snprintf zero padded left, pad count");
	assert(strcmp(buf, "123  ") == 0,
	       "snprintf zero padded left, pad mismatch");

	count = snprintf(buf, 1000, "%5d", 1234567);
	assert(count == 7, "incorrect snprintf padded right, no pad count");
	assert(strcmp(buf, "1234567") == 0,
	       "snprintf padded right, no pad mismatch");

	count = snprintf(buf, 1000, "%-5d", 1234567);
	assert(count == 7, "incorrect snprintf padded left, no pad count");
	assert(strcmp(buf, "1234567") == 0,
	       "snprintf padded left, no pad mismatch");

	count = snprintf(buf, 1000, "%05d", 1234567);
	assert(count == 7,
	       "incorrect snprintf zero padded right, no pad count");
	assert(strcmp(buf, "1234567") == 0,
	       "snprintf padded right, no pad mismatch");

	count = snprintf(buf, 1000, "%5d", -123);
	assert(count == 5, "incorrect snprintf neg padded right, pad count");
	assert(strcmp(buf, " -123") == 0,
	       "snprintf neg padded right, pad mismatch");

	count = snprintf(buf, 1000, "%05d", -123);
	assert(count == 5,
	       "incorrect snprintf neg zero padded right, pad count");
	assert(strcmp(buf, "-0123") == 0,
	       "snprintf neg zero padded right, pad mismatch");

	count = snprintf(buf, 1000, "%-5d", -123);
	assert(count == 5, "incorrect snprintf neg padded left, pad count");
	assert(strcmp(buf, "-123 ") == 0,
	       "snprintf neg padded left, pad mismatch");

#pragma GCC diagnostic ignored "-Wformat"
	count = snprintf(buf, 1000, "%-05d", -123);
#pragma GCC diagnostic pop
	assert(count == 5,
	       "incorrect snprintf zero neg padded left, pad count");
	assert(strcmp(buf, "-123 ") == 0,
	       "snprintf neg zero padded left, pad mismatch");

	count = snprintf(buf, 1000, "%5d", -1234567);
	assert(count == 8, "incorrect snprintf neg padded right, no pad count");
	assert(strcmp(buf, "-1234567") == 0,
	       "snprintf neg padded right, no pad mismatch");

	count = snprintf(buf, 1000, "%-5d", -1234567);
	assert(count == 8, "incorrect snprintf neg padded left, no pad count");
	assert(strcmp(buf, "-1234567") == 0,
	       "snprintf neg padded left, no pad mismatch");

	count = snprintf(buf, 1000, "%05d", -1234567);
	assert(count == 8,
	       "incorrect snprintf neg zero padded right, no pad count");
	assert(strcmp(buf, "-1234567") == 0,
	       "snprintf neg padded right, no pad mismatch");

	count = snprintf(buf, 1000, "%5x", 0xde);
	assert(count == 5, "incorrect snprintf hex padded right, pad count");
	assert(strcmp(buf, "   de") == 0,
	       "snprintf hex padded right, pad mismatch");

	count = snprintf(buf, 1000, "%05x", 0xde);
	assert(count == 5,
	       "incorrect snprintf zero hex padded right, pad count");
	assert(strcmp(buf, "000de") == 0,
	       "snprintf zero hex padded right, pad mismatch");

	count = snprintf(buf, 1000, "%-5x", 0xde);
	assert(count == 5, "incorrect snprintf hex padded left, pad count");
	assert(strcmp(buf, "de   ") == 0,
	       "snprintf hex padded left, pad mismatch");

	count = snprintf(buf, 1000, "%5x", 0xdeadbeef);
	assert(count == 8, "incorrect snprintf hex padded right, no pad count");
	assert(strcmp(buf, "deadbeef") == 0,
	       "snprintf hex padded right, no pad mismatch");

	count = snprintf(buf, 1000, "%05x", 0xdeadbeef);
	assert(count == 8,
	       "incorrect snprintf zero hex padded right, no pad count");
	assert(strcmp(buf, "deadbeef") == 0,
	       "snprintf zero hex padded right, no pad mismatch");

	count = snprintf(buf, 1000, "%-5x", 0xdeadbeef);
	assert(count == 8, "incorrect snprintf hex padded left, no pad count");
	assert(strcmp(buf, "deadbeef") == 0,
	       "snprintf hex padded left, no pad mismatch");

	count = snprintf(buf, 1000, "%10s", "hey!");
	assert(count == 10,
	       "incorrect snprintf string padded right, pad count");
	assert(strcmp(buf, "      hey!") == 0,
	       "snprintf string padded right, pad mismatch");

#pragma GCC diagnostic ignored "-Wformat"
	count = snprintf(buf, 1000, "%010s", "hey!");
#pragma GCC diagnostic pop
	assert(count == 10,
	       "incorrect snprintf zero string padded right, pad count");
	assert(strcmp(buf, "      hey!") == 0,
	       "snprintf zero string padded right, pad mismatch");

	count = snprintf(buf, 1000, "%-10s", "hey!");
	assert(count == 10, "incorrect snprintf string padded left, pad count");
	assert(strcmp(buf, "hey!      ") == 0,
	       "snprintf string padded left, pad mismatch");

	// Edge cases:

	// If we specify a buffer size of 5 we expect to actually output 4
	// characters.
	count = snprintf(buf, 5, "hello");
	// snprintf should return the number of characters that would be output
	// if the buffer were large enough.
	assert(count == 5, "incorrect truncated snprintf fmt count");
	assert(strcmp(buf, "hell") == 0, "truncated snprintf fmt mismatch");
	// Same thing but via a %s.
	count = snprintf(buf, 5, "%s", "hello");
	assert(count == 5, "incorrect truncated snprintf string count");
	assert(strcmp(buf, "hell") == 0, "truncated snprintf string mismatch");
	// Same thing but a digit.
	count = snprintf(buf, 5, "%d", 123456);
	assert(count == 6, "incorrect truncated snprintf int count");
	assert(strcmp(buf, "1234") == 0, "truncated snprintf int mismatch");
	// Same thing but a negative digit.
	count = snprintf(buf, 5, "%d", -123456);
	assert(count == 7, "incorrect truncated snprintf neg int count");
	assert(strcmp(buf, "-123") == 0, "truncated snprintf neg int mismatch");
	// Same thing but a padded right digit.
	count = snprintf(buf, 5, "%8d", 123456);
	assert(count == 8,
	       "incorrect truncated snprintf padded right int count");
	assert(strcmp(buf, "  12") == 0,
	       "truncated snprintf padded right int mismatch");
	// Same thing but a padded left digit.
	count = snprintf(buf, 5, "%-8d", 123456);
	assert(count == 8,
	       "incorrect truncated snprintf padded left int count");
	assert(strcmp(buf, "1234") == 0,
	       "truncated snprintf padded left int mismatch");

	// Zero size buffer should not touch the buffer at all.
	count = snprintf(buf, 0, "hello");
	assert(count == 0, "incorrect zero buffer snprintf count");
	// The buffer should not be touched.
	assert(strcmp(buf, "1234") == 0,
	       "zero buffer snprintf unexpected buffer updated");

	// Empty format string should result in empty buffer.
#pragma GCC diagnostic ignored "-Wformat-zero-length"
	count = snprintf(buf, 1000, "");
#pragma GCC diagnostic pop
	assert(count == 0, "incorrect empty format snprintf count");
	assert(strcmp(buf, "") == 0, "empty format snprintf string mismatch");

	return NULL;
}

const char *test_format(void)
{
	const char *res;

	res = test_snprintf();
	if (res != NULL)
		return res;

	return NULL;
}
