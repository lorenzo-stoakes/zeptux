#pragma once

#include "compiler.h"
#include "mem.h"
#include "types.h"

// See https://uefi.org/specs/ACPI/6.4/15_System_Address_Map_Interfaces/Sys_Address_Map_Interfaces.html#address-range-types
enum e820_type {
	E820_TYPE_RAM = 1,
	E820_TYPE_RESERVED = 2,
	E820_TYPE_ACPI = 3,
	E820_TYPE_NVS = 4,
	E820_TYPE_UNUSABLE = 5,
	E820_TYPE_DISABLED = 6,
	E820_TYPE_PERSISTENT = 7,
};

static inline const char *e820_type_to_string(enum e820_type type)
{
	switch (type) {
	case E820_TYPE_RAM:
		return "RAM";
	case E820_TYPE_RESERVED:
		return "reserved";
	case E820_TYPE_ACPI:
		return "ACPI";
	case E820_TYPE_NVS:
		return "NVS";
	case E820_TYPE_UNUSABLE:
		return "unusable";
	case E820_TYPE_DISABLED:
		return "disabled";
	case E820_TYPE_PERSISTENT:
		return "persistent";
	default:
		return "UNKNOWN";
	}
}

// See https://uefi.org/specs/ACPI/6.4/15_System_Address_Map_Interfaces/int-15h-e820h---query-system-address-map.html
struct e820_entry {
	uint64_t base;
	uint64_t size;
	uint32_t type;
} PACKED;

struct early_boot_info {
	uint64_t kernel_elf_size_bytes;
	uint16_t num_e820_entries;
	struct e820_entry e820_entries[0];
} PACKED;

static inline struct early_boot_info *boot_info()
{
	return (struct early_boot_info *)EARLY_BOOT_INFO_ADDRESS;
}
