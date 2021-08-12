#include "zeptux_early.h"

#define ROWS (25)
#define COLS (80)
#define MEMORY_SIZE (2 * ROWS * COLS)
#define BUFFER_SIZE (MEMORY_SIZE + 2 * COLS) // 1 extra row.

// This is not re-entrant but we're early and single core so it's fine.
static int curr_row = 0, curr_col = 0;

// Retrieve a pointer to the video buffer which, as this is called before any
// allocator is online, is an arbitrarily assigned area of memory.
static char *get_buf(void)
{
	char *buf = (char *)EARLY_VIDEO_BUFFER_ADDRESS;
	uint64_t index = 2 * (curr_row * COLS + (curr_col % COLS));

	return &buf[index];
}

// Output a character to the video buffer (but not screen, this happens on
// newline) if the line isn't yet truncated, if it is then output nothing except
// a '.' in the last column.
static void putc(char chr)
{
	// We truncate at the end of the line.
	if (curr_col == COLS - 2)
		chr = '.';
	else if (curr_col > COLS - 2)
		goto exit;

	char *buf = get_buf();
	// GCC gets ridiculously confused about this.
#pragma GCC diagnostic ignored "-Wstringop-overflow"
	buf[0] = chr;
	buf[1] = 15;
#pragma GCC diagnostic pop
exit:
	curr_col++;
}

// Scroll the memory buffer down one line.
static void scroll_down(void)
{
	// Copy from 2nd row to top of buffer, including hidden row to scroll
	// up.
	char *target = (char *)EARLY_VIDEO_BUFFER_ADDRESS;
	char *src = &target[2 * COLS];
	memcpy(target, src, MEMORY_SIZE);
	// Clear hidden row.
	memset(&target[MEMORY_SIZE], 0, 2 * COLS);
}

// Flush the video buffer to video memory, outputting the results to screen.
static void flush(void)
{
	volatile char *tgt = (volatile char *)EARLY_VIDEO_MEMORY_ADDRESS;
	char *src = (char *)EARLY_VIDEO_BUFFER_ADDRESS;

	for (int i = 0; i < MEMORY_SIZE; i++) {
		*tgt++ = *src++;
	}
}

// Handle a newline - we're line-buffered so this flushes to video memory. This
// also scrolls if necessary.
static void newline(void)
{
	if (curr_row == ROWS)
		scroll_down();
	else
		curr_row++;

	curr_col = 0;
	// We are line-buffered.
	flush();
}

// Simulate tab output.
static void puttab(void)
{
	for (int i = 0; i < 8; i++) {
		putc(' ');
	}
}

// Clear the area of memory we have assigned to the video buffer which we will
// be copying to video memory.
void early_video_init(void)
{
	char *buf = (char *)EARLY_VIDEO_BUFFER_ADDRESS;

	memset(buf, 0, BUFFER_SIZE);
}

// Put string to video buffer as best we can.
void early_video_puts(const char *str)
{
	if (str == NULL)
		return;

	for (char chr = *str; chr != '\0'; chr = *(++str)) {
		switch (chr) {
		case '\n':
			newline();
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
