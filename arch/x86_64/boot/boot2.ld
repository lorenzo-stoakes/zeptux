OUTPUT_FORMAT("binary")

SECTIONS {
	. = 0x7e00;           /* Load immediately after bootsector. */
	.text : ALIGN(512) {
	      *(.*)
	      . = 512 * 4;    /* We load 4 sectors so provide at least as many. */
	} = 0
	. = 0x7e00 + 512 * 4; /* Assert that we do not exceed allocated space. */
}
