#include "zeptux_early.h"

#define ROWS (25)
#define COLS (80)

// Not reentrant - we update this pointer to character position.
static volatile char *video_buf = (volatile char *)EARLY_VIDEO_MEMORY_ADDRESS;

static void move_top_left(void)
{
	video_buf = (volatile char *)EARLY_VIDEO_MEMORY_ADDRESS;
}

// Get _character_ offset into video buffer.
static uint64_t get_offset(void)
{
	// There are 2 bytes for every character.
	uint64_t twice = (uint64_t)video_buf - EARLY_VIDEO_MEMORY_ADDRESS;
	return twice / 2;
}

// Offset by `offset` characters into video buffer. This assumes overflow checks
// have already been done.
static void offset_by(uint64_t offset)
{
	// There are 2 bytes for every character.
	video_buf += offset * 2;
}

// Move to next line or wrap around.
static void next_line(void)
{
	uint64_t offset = get_offset();
	uint64_t row = offset / COLS;

	if (row == ROWS - 1)
		move_top_left();
	else
		offset_by(COLS - (offset % COLS));
}

// Put character to video buffer, wrapping around if necessary.
static void putc(char chr)
{
	uint64_t offset = get_offset();

	// Wrap around.
	if (offset >= ROWS * COLS)
		move_top_left();

	*video_buf++ = chr;
	*video_buf++ = 15; // Output in white text.
}

// put tab to video buffer.
static void puttab(void)
{
	for (int i = 0; i < 2; i++) {
		putc(' ');
	}
}

// Put string to video buffer as best we can.
void early_video_puts(const char *str)
{
	if (str == NULL)
		return;

	// We can simply go ahead and output to the video memory buffer.
	// See https://wiki.osdev.org/Printing_To_Screen

	for (char chr = *str; chr != '\0'; chr = *(++str)) {
		switch (chr) {
		case '\n':
			next_line();
			break;
		case '\t':
			puttab();
			break;
		default:
			putc(chr);
			break;
		}
	}
}
