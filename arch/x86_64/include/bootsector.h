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
// 'flag' and 'access' define various properties of the entries, defined in
// X86_GDTE_FLAG_* and X86_GDTE_ACCESS_* respectively.

// We assume base = 0, limit = 0xfffff:
//                             b-------f---l---a-------b-----------------------l---------------
#define GDTE_TEMPLATE_ENTRY \
	(0b0000000000001111000000000000000000000000000000001111111111111111ULL)

// Generate GDTE based on the template above.
#define MAKE_GDTE(_flag, _access) \
	(GDTE_TEMPLATE_ENTRY | ((_flag) << 52) | ((_access) << 40))

// We define the indexes of our GDTEs (base-1) according to how we use them,
// which are treated like 'segments' in long jumps in protected/long mode.
#define GDT_SEGMENT_CODE32_INDEX (1)
#define GDT_SEGMENT_CODE64_INDEX (2)
#define GDT_SEGMENT_DATA_INDEX (3)

// When we reference the GDT we are using on long jump we have to specify it as
// a byte offset. We are using base-1 as 'segment' 0 cannot be selected, and we
// will have specified a zero GDTE which we skip over.
#define GDT_SEGMENT_CODE32 (GDT_SEGMENT_CODE32_INDEX * 8)
#define GDT_SEGMENT_CODE64 (GDT_SEGMENT_CODE64_INDEX * 8)
#define GDT_SEGMENT_DATA (GDT_SEGMENT_DATA_INDEX * 8)

// By definition the boot drive is always 0x80.
#define BOOT_DRIVE_NUM (0x80)

// The 2^ power for disk sector size.
#define SECTOR_SHIFT (9)

// The size of a disk sector in bytes (512).
#define SECTOR_SIZE_BYTES (1ULL << SECTOR_SHIFT)

// The number of 512-byte sectors the stage 2 boot loader uses.
#define STAGE2_SECTORS (4)

// The number of 512-byte sectors both stage 1 and 2 boot loaders use.
#define BOOT_SECTORS (1 + STAGE2_SECTORS)

// The location where the BIOS loads the kernel ELF image (later moved into the
// correct place after transitioning from real mode).
#define BIOS_KERNEL_ELF_LOAD_PHYS_ADDRESS \
	(0x7c00 + BOOT_SECTORS * SECTOR_SIZE_BYTES)

// Remaining sectors in the segment we first load the BIOS ELF image into.
#define BIOS_KERNEL_ELF_LOAD_SEGMENT0_SECTORS_REMAINING \
	((0x10000 - BIOS_KERNEL_ELF_LOAD_PHYS_ADDRESS) >> SECTOR_SHIFT)

// Where we load the DAP for int 0x13 ah=0x42 disk loading.
#define BIOS_KERNEL_ELF_LOAD_DAP_PHYS_ADDRESS (0x500)

// The location where the ELF image size is encoded by a script.
#define KERNEL_IMAGE_SIZE_MEM_ADDRESS (BIOS_KERNEL_ELF_LOAD_PHYS_ADDRESS - 4)
