#include "early_io.h"
#include "zeptux.h"

static struct kernel_log_state *state;

void kernel_log_init(void)
{
	uint64_t bytes =
		sizeof(struct kernel_log_state) +
		KERNEL_LOG_MAX_NUM_ENTRIES * sizeof(struct kernel_log_entry);
	state = kzalloc(bytes, KMALLOC_KERNEL);
}

// Get pointer to next write kernel log entry.
// ASSUMES: `state->lock` held.
static struct kernel_log_entry *next_write_entry(void)
{
	// If we have entries < max, need to increment entry count and write offset.
	if (state->num_entries < KERNEL_LOG_MAX_NUM_ENTRIES) {
		state->num_entries++;
		return &state->entries[state->write_offset++];
	}

	// Otherwise, read and write offset move in lockstep.

	struct kernel_log_entry *ret = &state->entries[state->write_offset];

	state->write_offset =
		(state->write_offset + 1) % KERNEL_LOG_MAX_NUM_ENTRIES;
	state->read_offset =
		(state->read_offset + 1) % KERNEL_LOG_MAX_NUM_ENTRIES;

	return ret;
}

// Echo log output if kernel configured to do so.
static void maybe_echo_log(struct kernel_global *global, log_flags_t flags,
			   const char *buf)
{
	if (!global->log_echo)
		return;

	// For now we only support early mode.
	switch (global->stage) {
	case KERNEL_STAGE_1_EARLY:
		break;
	default:
		panic("Unsupported kernel stage %d", global->stage);
	}

	switch (flags & KERNEL_LOG_MASK) {
	case KERNEL_LOG_TRACE:
		early_printf("TRACE: ");
		break;
	case KERNEL_LOG_DEBUG:
		early_printf("DEBUG: ");
		break;
	case KERNEL_LOG_INFO:
		early_printf("INFO:  ");
		break;
	case KERNEL_LOG_WARN:
		early_printf("WARN:  ");
		break;
	case KERNEL_LOG_ERROR:
		early_printf("ERROR: ");
		break;
	case KERNEL_LOG_CRITICAL:
		early_printf("CRIT:  ");
		break;
	}

	early_printf("%s\n", buf);
}

// Add kernel log entry to kernel ring buffer.
void log_vprintf(log_flags_t flags, const char *fmt, va_list ap)
{
	spinlock_acquire(&state->lock);
	struct kernel_global *global = global_get_locked();

	// Do we need to log?
	if ((flags & KERNEL_LOG_MASK) < (global->log_level & KERNEL_LOG_MASK))
		goto done;

	struct kernel_log_entry *entry = next_write_entry();
	// TODO: Add support for timestamps.
	entry->flags = flags;
	vsnprintf(entry->buf, KERNEL_LOG_BUF_SIZE, fmt, ap);
	maybe_echo_log(global, flags, entry->buf);

done:
	spinlock_release(&global->lock);
	spinlock_release(&state->lock);
}

void log_printf(log_flags_t flags, const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	log_vprintf(flags, fmt, list);
	va_end(list);
}
