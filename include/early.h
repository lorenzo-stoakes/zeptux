/*
 * Contains routines for early in the boot procedure where:
 *
 *   - We cannot handle interrupts.
 *   - We can't sleep.
 *   - We either have no allocator or the simple boot allocator.
 */

#pragma once

void early_serial_init_poll(void);
void early_serial_putc(char chr);
void early_serial_putstr(const char *str);
