#include "../include/termfeatures.h"

void term_get_size(int* row, int* cols) {
	printf("\033[s"); // save cursor position
	printf("\033[999;999H"); // move cursor far away
	fflush(stdout);
	term_raw_enable(true);
	printf("\033[6n"); // Device Status Report
	fflush(stdout);
	printf("\033[u"); // restore cursor position
	fflush(stdout);
	scanf("\033[%d;%dR", row, cols);
	term_raw_enable(false);
}

void term_save_cursor() {
	printf("\033[s");
}
void term_restore_cursor() {
	printf("\033[u");
}

void term_default_colors() {
	printf("\033[49m");
	printf("\033[39m");
}

void term_clear() {
	printf("\033[2J");
}

void term_move_cursor(int row, int column) {
	printf("\033[%d;%dH", row, column);
}

void term_set_bg(int r, int g, int b, bool hq) {
	if (hq) {
		printf("\033[48;2;%d;%d;%dm", r, g, b);
	} else {
		r = r/51;
		g = g/51;
		b = b/51;
		int data = 16+36*r+6*g+b;
		printf("\033[48;5;%dm", data);
	}
}

void term_set_fg(int r, int g, int b, bool hq) {
	if (hq) {
		printf("\033[38;2;%d;%d;%dm", r, g, b);
	} else {
		r = r/51;
		g = g/51;
		b = b/51;
		int data = 16+36*r+6*g+b;
		printf("\033[38;5;%dm", data);
	}
}

void term_raw_enable(bool enable) {
	if (enable) {
		_ioctl(0, IOCTL_TERMMODE, TERMMODE_RAW);
	} else {
		_ioctl(0, IOCTL_TERMMODE, TERMMODE_CANON);
	}
}
