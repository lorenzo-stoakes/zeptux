#pragma once

#include "mem.h"
#include "types.h"

struct early_boot_info {
	uint64_t kernel_elf_size_bytes;
};

struct early_boot_info *boot_info()
{
	return (struct early_boot_info *)EARLY_BOOT_INFO_ADDRESS;
}
