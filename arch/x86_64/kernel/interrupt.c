#include "zeptux.h"

#define APIC_IRQ_0 (32) // Offset to avoid conflict with errors.
#define APIC_IRQ_TIMER (0)
#define APIC_IRQ_ERROR (0x13)
#define APIC_IRQ_SPURIOUS (0xdf) // Lower bits must be set.

#define APIC_VER_OFFSET (0x30)	     // APIC version information.
#define APIC_TPR_OFFSET (0x80)	     // Task Priority Register.
#define APIC_EOI_OFFSET (0x0b0)	     // End Of Interrupt register.
#define APIC_SPURIOUS_OFFSET (0x0f0) // For specifying spurious interrupt vector.
#define APIC_ESR_OFFSET (0x280)	     // Error Status Register
#define APIC_TIMER_OFFSET (0x320)    // Local timer.
#define APIC_LINT0_OFFSET (0x350)    // Local interrupt pin 0 register.
#define APIC_LINT1_OFFSET (0x360)    // Local interrupt pin 1 register.
#define APIC_ERROR_OFFSET (0x370)    // Error interrupt register.
#define APIC_TICR_OFFSET (0x380)     // Timer Initial Count Register.
#define APIC_TDCR_OFFSET (0x3e0)     // Timer Divide Config Register.

#define APIC_ENABLE_FLAG (0x100)     // Set bit to enable an interrupt.
#define APIC_MASK_FLAG (0x10000)     // Set bit to mask an interrupt.
#define APIC_PERIODIC_FLAG (0x20000) // Set bit to indicate value periodic.

#define APIC_INIT_TIMER_COUNT_VALUE (1000000000)

// The maximum number of entries in the Local Vector Table, see intel manual 3a,
// section 10.5.1.
static uint32_t num_lvt_entries;

// Obtain pointer to APIC register.
static volatile uint32_t *apic_reg_ptr(uint64_t offset)
{
	return (volatile uint32_t *)(APIC_BASE_ADDRESS + offset);
}

// Read from APIC register.
static uint32_t read_apic_reg(uint64_t offset)
{
	return *apic_reg_ptr(offset);
}

// Write to APIC register.
static void write_apic_reg(uint64_t offset, uint32_t val)
{
	volatile uint32_t *ptr = apic_reg_ptr(offset);
	*ptr = val;
}

// Clear error status.
static void clear_error(void)
{
	// Requires 2 writes.
	write_apic_reg(APIC_ESR_OFFSET, 0);
	write_apic_reg(APIC_ESR_OFFSET, 0);
}

// Mark an interrupt cleared.
static void clear_interrupt(void)
{
	write_apic_reg(APIC_EOI_OFFSET, 0);
}

// Initialise the local APIC.
static void lapic_init(void)
{
	// Retrieve the number of local vector table entries. See intel manual
	// 3a, figure 10-7.
	uint32_t ver = read_apic_reg(APIC_VER_OFFSET);
	uint32_t max_lvt_entries = (ver & BIT_MASK_BELOW(23)) >> 16;
	num_lvt_entries = max_lvt_entries + 1;

	// Disable the local interrupt pins, these appear only to be used in
	// conjunction with the legacy PIC (we've disabled that) and sometimes
	// for NMIs... but probably not in a multicore system.
	// TODO: Research this further.
	write_apic_reg(APIC_LINT0_OFFSET, APIC_MASK_FLAG);
	write_apic_reg(APIC_LINT1_OFFSET, APIC_MASK_FLAG);

	// Clear any error status.
	clear_error();
	// Clear any residual interrupts.
	clear_interrupt();

	// Set error interrupt IRQ.
	write_apic_reg(APIC_ERROR_OFFSET, APIC_IRQ_0 + APIC_IRQ_ERROR);

	// Configure the timer.
	write_apic_reg(APIC_TDCR_OFFSET, 0xb); // Divide counts by 1.
	write_apic_reg(APIC_TIMER_OFFSET,
		       APIC_PERIODIC_FLAG | (APIC_IRQ_0 + APIC_IRQ_TIMER));
	// We count down from this.
	write_apic_reg(APIC_TICR_OFFSET, APIC_INIT_TIMER_COUNT_VALUE);

	// Enable local APIC (step 1) - specify spurious interrupt.
	// See https://en.wikipedia.org/wiki/Interrupt#Spurious_interrupts
	write_apic_reg(APIC_SPURIOUS_OFFSET,
		       APIC_ENABLE_FLAG | (APIC_IRQ_0 + APIC_IRQ_SPURIOUS));
	// Enable local APIC (step 2) - Set task priority register which
	// determines the MINIMUM interrupt priority we want the APIC to
	// process. By setting to 0, we enable all external interrupts.
	// NOTE: This enables interrupts on the APIC, not the CPU.
	write_apic_reg(APIC_TPR_OFFSET, 0);
}

void interrupt_init(void)
{
	lapic_init();
}
