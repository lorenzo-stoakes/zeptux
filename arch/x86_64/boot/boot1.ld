OUTPUT_FORMAT("binary")

ENTRY(start)
SECTIONS {
	. = 0x7c00;		/* In x86, the MBR is loaded at 0x7c00. */
	.text : {
	      *(.*)		/* Everything goes into .text section. */
	      . = 512 - 2;
	      SHORT(0xaa55);	/* Place magical MBR bytes in correct place. */
	      . = ALIGN(512);	/* Bootsector is 512 bytes long. */
	} = 0			/* Zero the rest of the bootsector. */
}

boot2 = 0x7e00;
