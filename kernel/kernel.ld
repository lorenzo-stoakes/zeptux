ENTRY(main)

SECTIONS {
	/* KERNEL_TEXT_ADDRESS */
	. = 0xffffc80001001000;

	.text : ALIGN(4K) {
		*(.text .text.*)
	}

	.rodata : ALIGN(4K) {
		*(.rodata .rodata.*)
	}

	.data : ALIGN(4K) {
		*(.data .data.*)
	}

	.bss : ALIGN(4K) {
		*(.bss .bss.*)
	}

	 /DISCARD/ : {
		*(.comment .note .eh_frame .note.GNU-stack .note.gnu.property)
	 }
}
