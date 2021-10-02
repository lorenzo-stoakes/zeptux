#pragma once

#include "compiler.h"
#include "spinlock.h"
#include "types.h"

#define KERNEL_LOG_MAX_NUM_ENTRIES (1023)
#define KERNEL_LOG_BUF_SIZE (500)

// Flags describing kernel log flags, specifically log level of an entry.
typedef enum {
	KERNEL_LOG_TRACE = 0,
	KERNEL_LOG_DEBUG = 1,
	KERNEL_LOG_INFO = 2,
	KERNEL_LOG_WARN = 3,
	KERNEL_LOG_ERROR = 4,
	KERNEL_LOG_CRITICAL = 5,
	KERNEL_LOG_MASK = BIT_MASK_BELOW(10),
	KERNEL_LOG_DUMMY_UINT32 = 0xffffffff,
} log_flags_t;
static_assert(sizeof(log_flags_t) == sizeof(uint32_t));

// Represents a kernel log entry. These are all fixed sized.
struct kernel_log_entry {
	uint64_t timestamp;
	log_flags_t flags;
	char buf[KERNEL_LOG_BUF_SIZE];
};
static_assert(sizeof(struct kernel_log_entry) == 512);

// Represents kernel log entries contained within a ring buffer.
struct kernel_log_state {
	uint64_t num_entries;
	uint64_t read_offset, write_offset;
	spinlock_t lock;
	struct kernel_log_entry entries[];
};

// Initialise kernel log state.
void kernel_log_init(void);

// Retrieve kernel log state and acquire lock.
struct kernel_log_state *kernel_log_get_state_lock(void);

// Add kernel log entry to kernel ring buffer.
void PRINTF(2, 0) log_vprintf(log_flags_t flags, const char *fmt, va_list ap);

// Write entry to the kernel ring buffer log.
void PRINTF(2, 3) log_printf(log_flags_t flags, const char *fmt, ...);

// Write entry to the kernel ring buffer log at TRACE level.
static inline void PRINTF(1, 2) log_trace(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(KERNEL_LOG_TRACE, fmt, list);
	va_end(list);
}

// Write entry to the kernel ring buffer log at DEBUG level.
static inline void PRINTF(1, 2) log_debug(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(KERNEL_LOG_DEBUG, fmt, list);
	va_end(list);
}

// Write entry to the kernel ring buffer log at INFO level.
static inline void PRINTF(1, 2) log_info(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(KERNEL_LOG_INFO, fmt, list);
	va_end(list);
}

// Write entry to the kernel ring buffer log at WARN level.
static inline void PRINTF(1, 2) log_warn(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(KERNEL_LOG_WARN, fmt, list);
	va_end(list);
}

// Write entry to the kernel ring buffer log at ERROR level.
static inline void PRINTF(1, 2) log_error(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(KERNEL_LOG_ERROR, fmt, list);
	va_end(list);
}

// Write entry to the kernel ring buffer log at CRITICAL level.
static inline void PRINTF(1, 2) log_critical(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(KERNEL_LOG_CRITICAL, fmt, list);
	va_end(list);
}
