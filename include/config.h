#pragma once

// Initial implementation of kernel configuration. We simply put defines here
// and import config.h where config options are used.

// The kernel is loaded using ATA commands via PIO. This won't work on all
// hardware, but qemu is fine with it.
//#define CONFIG_BOOTLOADER_ATA_PIO

// The kernel is loaded using a BIOS interrupt call. This will almost certainly
// work basically on anything but we are limited as to the size of kernel ELF
// image we can load.
#define CONFIG_BOOTLOADER_BIOS

#if defined(CONFIG_BOOTLOADER_ATA_PIO) && defined(CONFIG_BOOTLOADER_BIOS)
#error CONFIG_BOOTLOADER_ATA_PIO and CONFIG_BOOTLOADER_BIOS are mutually exclusive!
#endif

#if !defined(CONFIG_BOOTLOADER_ATA_PIO) && !defined(CONFIG_BOOTLOADER_BIOS)
#error At least 1 CONFIG_BOOTLOADER_* option must be specified.
#endif
