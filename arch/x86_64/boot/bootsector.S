.code16
.text
.globl start
start:
	cli			# We can't handle being interrupted.
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss		# Clear DS, ES, SS as these could contain junk.

	movw $hello_msg, %si	# Load message into SI (source) register.
	movb $0xe, %ah		# We intend to call interrupt 0x10, function
				# 0xe e.g. 'teletype output'.
				# See https://en.wikipedia.org/wiki/INT_10H
	xor %bx, %bx		# We don't want to specify page/colour.
loop:
	lodsb			# Loads next byte from DS:SI into AL.
	cmpb $0, %al		# If we have reached the end of the string,
	je bye			# exit.
	int $0x10		# Write character using BIOS.
	jmp loop
bye:
	hlt

hello_msg:
	.asciz "Hello, World! "	# Trailing space as qemu puts cursor there.
