#include "asm.h"
#include "early_serial.h"
#include "types.h"

#define COM1 (0x3f8)
#define BAUD_RATE (115200)
#define BASE_BAUD_RATE (115200)
#define DLAB_FLAG (0x80)
#define MODE_8N1 (3)
#define LSR_THR (0x20)

static bool is_transmit_empty()
{
	return inb(COM1 + 5) & LSR_THR;
}

void early_serial_init_poll(void)
{
	// We need to specify ratio betwen baud rate and the base crystal
	// oscillation rate of a 16550 UART serial chip.
	uint16_t ratio = BAUD_RATE / BASE_BAUD_RATE;

	// See https://wiki.osdev.org/Serial_Ports#Initialization
	outb(COM1 + 1, 0); // Disable interrupts.
	outb(COM1 + 3,
	     DLAB_FLAG);   // Enable DLAB - 'Divisor Latch Access Bit'. Lets
			   // us set baud rate.
	outb(COM1, ratio); // Baud low bits.
	outb(COM1 + 1, ratio >> 8); // Baud high bits.
	outb(COM1 + 3, MODE_8N1);   // Set 8 bits, no parity, 1 stop bit.
	outb(COM1 + 4, 0);	    // Disable IRQs.

	// ASSUME: It works correctly.
}

void early_serial_putc(char chr)
{
	while (!is_transmit_empty())
		;

	outb(COM1, (uint8_t)chr);
}

void early_serial_puts(const char *str)
{
	for (const char *chr = str; *chr != '\0'; chr++) {
		early_serial_putc(*chr);
	}
}
