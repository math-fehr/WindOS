#ifndef TERMFEATURES_H
#define TERMFEATURES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "syscalls.h"


#define FB_WIDTH 	0
#define FB_HEIGHT 	1
#define FB_OPEN 	2
#define FB_CLOSE 	3
#define FB_SHOW 	4
#define FB_FLUSH 	5
#define FB_BUFFERED 6

#define IOCTL_TERMMODE	5000
#define IOCTL_BLOCKING 	10000

#define TERMMODE_RAW	0
#define TERMMODE_CANON 	1


void term_get_size(int* row, int* cols);
void term_clear();
void term_move_cursor(int row, int column);
void term_set_bg(int r, int g, int b, bool hq);
void term_set_fg(int r, int g, int b, bool hq);
void term_raw_enable(bool enable);
void term_save_cursor();
void term_restore_cursor();
void term_default_colors();

void get_permission_string(mode_t attr, char* buf);

#endif
