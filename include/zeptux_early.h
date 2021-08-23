#pragma once

// Convenience header that pulls in all required headers for the early boot
// stage (i.e. pre-allocator, pre-interrupt, etc. support).

#include "early_boot_info.h"
#include "early_init.h"
#include "early_io.h"
#include "early_mem.h"
#include "early_serial.h"
#include "early_video.h"

#include "zeptux.h"
