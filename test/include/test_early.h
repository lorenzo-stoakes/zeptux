#pragma once

#include "test_helpers.h"
#include "zeptux_early.h"

// Since we know we're running in qemu we can make assumptions about how we quit
// it.
static inline void exit_qemu(void)
{
	// This is equivalent to an ACPI shutdown command. Ordinarily you have
	// to look the port and data fields up using ACPI tables, however since
	// we're in qemu we can make some rash assumptions here!
	outw(0x604, 0x2000);
}

// test_format.c
const char *test_format(void);

// test_string.c
const char *test_string(void);

// test_misc.c
const char *test_misc(void);

// test_mem.c
const char *test_mem(void);
