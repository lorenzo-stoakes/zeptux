ENTRY(main)

SECTIONS {
	/* KERNEL_TEXT_ADDRESS */
	. = 0xffffc00001001000;

	.text : ALIGN(4K) {
		*(.text .text.*)
	}

	.rodata : ALIGN(4K) {
		*(.rodata .rodata.*)
	}

	.data : ALIGN(4K) {
		*(.data .data.*)
	}

	/*
	 * Offset to ensure that the section headers do not overlap with .bss in
	 * virtual memory.
	 */
	. += 0x10000;

	.bss : ALIGN(4K) {
		*(.bss .bss.*)
	}

	 /DISCARD/ : {
		*(.comment .note .eh_frame .note.GNU-stack .note.gnu.property)
	 }
}
