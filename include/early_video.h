#pragma once

// Clear the area of memory we have assigned to the video buffer which we will
// be copying to video memory.
void early_video_init(void);
// Put string to video buffer as best we can.
void early_video_puts(const char *str);
