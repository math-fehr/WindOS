#ifndef TERMFEATURES_H
#define TERMFEATURES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define IOCTL_TERMMODE	0

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

#endif
