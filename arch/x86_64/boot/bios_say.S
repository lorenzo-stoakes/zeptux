// Outputs string pointed to by SI.
bios_say:
	// 'teletype output' - see https://en.wikipedia.org/wiki/INT_10H
	movb $0xe, %ah
	xorw %bx, %bx
bios_say.loop:
	lodsb // Load byte at DS:SI into AL.
	cmpb $0, %al
	je bios_say.done // Exit on null terminator.
	int $0x10
	jmp bios_say.loop
bios_say.done:
	ret
