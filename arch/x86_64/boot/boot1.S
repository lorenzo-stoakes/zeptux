.text
.code16

.globl start
.globl boot2 // Declared in boot1.ld

// Stage 1 bootloader

#include "x86-consts.h"
#include "bootsector.h"

start:
	// Normalise CS.
	ljmp $0, $start16
start16:
	/*
	 * We can't handle being interrupted and the registers might be in an
	 * invalid state at the outside, so clear them all.
	 */
	cli
	xorw %ax, %ax
	movw %ax, %ds // Data  segment.
	movw %ax, %es // Extra segment.
	movw %ax, %ss // Stack segment.

	movw $X86_EARLY_INIT_STACK, %sp // Very early stack.

	call enable_a20

	call check_long_mode_support
	cmpw $1, %ax
	jne start.no_long_mode

	call load_stage2
	jmp boot2

start.no_long_mode:
	movw $panic_nolong_str, %si
	call bios_say
start.forever:
	jmp start.forever

/*
 * Once upon a time 1 MiB was considered a huge amount of memory. Thus the
 * weirdness that is segmented memory meant that 16-bit x86 could specify an
 * address exceeding this 'huge' capacity, e.g. 0xffff:0xffff = 0x10ffef =
 * 0x100000 (1 MiB) + 0xffef (just under 64 KiB).
 *
 * Intel thought it'd be a pretty great idea to round things out at 1 MiB by
 * 'wrapping' memory around so anything exceeding 0xffff:0x0010 would
 * 'wrap around' to 0x0000:0x0000.
 *
 * Inevitably some developers decided that it was a great idea to rely on this
 * behaviour, which left Intel with the Old New Thing problem of maintaining
 * backwards compatibility in newer processors.
 *
 * The ability to enable/disable A20 is the solution - it refers to 'address
 * line 20' (in base 0, so 21st bit), e.g. if enabled then no wrap-around,
 * otherwise wrap-around.
 *
 * For compatibility (some) x86 processors start real mode with A20 disabled.
 * For obvious reasons we have to enable it.
 *
 * For more on this horror read https://www.win.tue.nl/~aeb/linux/kbd/A20.html
 */
enable_a20:
	// Is A20 already enabled? Then nothing to do.
	call check_a20
	cmpw $1, %ax
	je enable_a20.done

	// Attempt 1: BIOS enable.
	movw $0x2401, %ax
	int $0x15
	call check_a20
	cmpw $1, %ax
	je enable_a20.done

	// Attempt 2: Fast A20.
	inb $0x92, %al
	orb $0b10, %al
	outb %al, $0x92
	call check_a20
	cmpw $1, %ax
	je enable_a20.done

	// Nothing worked, we cannot continue.
	movw $panic_a20_str, %si
	call bios_say
enable_a20.forever:
	jmp enable_a20.forever

enable_a20.done:
	ret

/*
 * Checks whether A20 is enabled. It achieves this by firstly checking if the
 * bootsector magic number, 0xaa55 (located at 0x0000:0x7dfe, e.g. bootsector
 * load address 0x7c00 + 512 - 2), differs from the A20-disabled wrap around at
 * 0xffff:0x7e0e. If they differ then A20 cannot be enabled. If by chance they
 * happen to be the same, we set 0x0000:0x7dfe to an arbitrary value and check
 * again. If they mirror one another then A20 is definitely disabled. The magic
 * number is restored afterwards.
 *
 * Sets AX to 1 if enabled, 0 if disabled.
 */
check_a20:
	// Clear DS.
	xorw %ax, %ax
	movw %ax, %ds
	// Set DI to 0x7dfe so %ds:%di == 0x0000: 0x7dfe.
	movw $0x7dfe, %di
	// Store 0x0000:0x7dfe in %ax and copy to %bx.
	movw %ds:0(%di), %ax
	movw %ax, %bx
	// Set ES to 0xffff.
	movw $0xffff, %ax
	movw %ax, %es
	// Set SI to 0x7e0e so %es:%si == 0xffff:0x7e0e.
	movw $0x7e0e, %si
	// If 0x0000:0x7dfe != 0xffff:0x7e0e then A20 must be enabled.
	cmpw %es:0(%si), %ax
	jne check_a20.enabled
	/*
	 * Just in case they happen to match by chance, set 0x0000:0x7dfe to an
	 * arbitrary value.
	 */
	movw $0xbeef, %ax
	movw %ax, %ds:0(%di)
	// Check if they match, if they do now then A20 is definitely disabled.
	cmpw %es:0(%si), %ax
	// Before returning, restore bootsector magic value.
	movw %bx, %ds:0(%di)
	je check_a20.disabled
check_a20.enabled:
	movw $1, %ax
	ret
check_a20.disabled:
	movw $0, %ax
	ret

/*
 * Determines whether CPU supports long mode (e.g. 64-bit).
 * Sets AX 1 if so, 0 if not.
 */
check_long_mode_support:
	xorl %edx, %edx
	movl $0x80000001, %eax
	cpuid
	andl $X86_LONGMODE_FLAG, %edx
	jz check_long_mode_support.unsupported
	movw $1, %ax
	ret
check_long_mode_support.unsupported:
	movw $0, %ax
	ret

// Uses BIOS to load the stage bootloader code into memory.
load_stage2:
	// Clear ES.
	xorw %ax, %ax
	movw %ax, %es
	// Read mode.
	movb $2, %ah
	movb $STAGE2_SECTORS, %al
	// Place them just after this block.
	movw $boot2, %bx
	// Start at sector 2 (skip this bootsector).
	movw $2, %cx
	// Read from boot drive.
	movw $BOOT_DRIVE_NUM, %dx
	int $0x13
	ret

#include "bios_say.S"

panic_a20_str:
	.asciz "panic: unable to enable a20!"

panic_nolong_str:
	.asciz "panic: long mode not supported!"
