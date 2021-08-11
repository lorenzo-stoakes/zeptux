OUTPUT_FORMAT("binary")

SECTIONS {
	. = 0x7e00;           /* Load immediately after bootsector. */
	.text : ALIGN(512) {
		*(.*)

		. = 0x7e00 + 512 * 4 - 4; /* Allow space for the image size to be inserted. */
	} = 0

	. = 0x7e00 + 512 * 4; /* Assert that we do not exceed allocated space. */
}
