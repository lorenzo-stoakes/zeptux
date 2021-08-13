#pragma once

// Initial kernel stack location.
#define EARLY_KERNEL_INIT_STACK (X86_KERNEL_INIT_STACK)
// Location where we store intial boot information useful for the kernel.
#define EARLY_BOOT_INFO_ADDRESS (X86_EARLY_BOOT_INFO_ADDRESS)
// Location in memory where text output can be written to which gets displayed
// on the monitor.
#define EARLY_VIDEO_MEMORY_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_VIDEO_MEMORY_ADDRESS_PHYS)
// Location in memory which acts as a double buffer to the actual video memory.
#define EARLY_VIDEO_BUFFER_ADDRESS \
	(KERNEL_DIRECT_MAP_BASE + X86_EARLY_VIDEO_BUFFER_ADDRESS_PHYS)
