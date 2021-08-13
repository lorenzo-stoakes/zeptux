#pragma once

/*
 * Convenience header that pulls in all required headers for the early boot
 * stage.
 */

#include "early_boot_info.h"
#include "early_init.h"
#include "early_io.h"
#include "early_mem.h"
#include "early_serial.h"
#include "early_video.h"

#include "asm.h"
#include "elf.h"
#include "macros.h"
#include "mem.h"
#include "spinlock.h"
#include "string.h"
#include "types.h"
#include "ver.h"
