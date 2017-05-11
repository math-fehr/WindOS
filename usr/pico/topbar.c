#include "pico.h"

extern int row;
extern int col;
extern char* 	path;

void topbar_draw() {
	term_save_cursor();
	term_move_cursor(1, 1);
	term_set_bg(255, 255, 255, false);
	term_set_fg(0, 0, 0, false);


	char msg[80];
	if (path[0] == 0) {
		sprintf(msg, "WindOS PICO - New file");
	} else {
		sprintf(msg, "WindOS PICO - %s", path);
	}
	int wlen = strlen(msg);
	for (int i=0;i<col;i++) {
		if (i == col/2 - wlen/2) {
			printf("%s", msg);
			i = col/2 + wlen/2;
		} else {
			printf(" ");
		}
	}
	term_default_colors();
	term_restore_cursor();
	fflush(stdout);
}
