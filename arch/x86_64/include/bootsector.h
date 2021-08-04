#pragma once

// Global Descriptor Tables specify the 32-bit segment in a rather awkward
// fashion: (See https://wiki.osdev.org/Global_Descriptor_Table)

// base     flag lim  access   base                     limit
//    6           5          4          3         2          1
// 32109876 5432 1098 76543210 987654321098765432109876 5432109876543210
//
// The 'base' value specifies the minimum address, and 'limit' the maximum
// addressable unit (either bytes or pages depending on flags).
//
// 'base' and 'limit' are split into separate groups of bits, from low to high.
//
// 'flag' specifies

// We assume base = 0, limit = 0xfffff:
//                             b-------f---l---a-------b-----------------------l---------------
#define GDTE_TEMPLATE_ENTRY (0b0000000000001111000000000000000000000000000000001111111111111111ULL)

#define MAKE_GDTE(_flag, _access) \
	(GDTE_TEMPLATE_ENTRY | ((_flag) << 52) | ((_access) << 40))

#define GDT_SEGMENT_CODE32_INDEX (1)
#define GDT_SEGMENT_CODE64_INDEX (2)
#define GDT_SEGMENT_DATA_INDEX   (3)

#define GDT_SEGMENT_CODE32 (GDT_SEGMENT_CODE32_INDEX * 8)
#define GDT_SEGMENT_CODE64 (GDT_SEGMENT_CODE64_INDEX * 8)
#define GDT_SEGMENT_DATA   (GDT_SEGMENT_DATA_INDEX   * 8)

// By definition the boot drive is always 0x80.
#define BOOT_DRIVE_NUM (0x80)

// The size of a disk sector in bytes.
#define SECTOR_SIZE_BYTES (512)

// The number of 512-byte sectors the stage 2 boot loader uses.
#define STAGE2_SECTORS (4)
