#pragma once

// Initialise early serial output. We assume COM1, baud rate of 115200 and 8N1
// mode. We operate in polling mode, disabling interrupts.
void early_serial_init_poll(void);
// Output a single character to serial output.
void early_serial_putc(char chr);
// Output a string to serial output.
void early_serial_puts(const char *str);
