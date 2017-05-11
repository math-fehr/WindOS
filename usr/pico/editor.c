#include "pico.h"


extern char** 	file;
extern int 		nlines;

extern int cursor_row;
extern int cursor_col;

void refresh_line(int n) {
	term_save_cursor();
	term_move_cursor(3+n, 1);
	printf("\033[2K");
	term_set_bg(255, 255, 255, false);
	term_set_fg(0, 0, 0, false);
	printf("% 3d", n+1);
	term_default_colors();
	printf(" %s\n", file[n]);
	term_restore_cursor();
}

void update_cursor() {
	term_move_cursor(cursor_row+3, cursor_col+5);
}

void editor_draw() {
	term_move_cursor(2, 1);
	term_set_bg(255, 255, 255, false);
	term_set_fg(0, 0, 0, false);
	printf("   \n");
	for (int i=0;i<nlines;i++) {
		term_set_bg(255, 255, 255, false);
		term_set_fg(0, 0, 0, false);
		printf("% 3d", i+1);
		term_default_colors();
		printf(" %s\n", file[i]);
	}
	fflush(stdout);
	cursor_row = 0;
	cursor_col = 0;
}

void editor_delete() {
	if (cursor_col == 0) {
		if (cursor_row == 0) {
			// do nothing
		} else {
			char** old_file = file;
			char** new_file = malloc(sizeof(char*)*(nlines));
			for (int i=0;i<nlines-1;i++) {
				if (i < cursor_row-1) {
					new_file[i] = old_file[i];
				} else if (i == cursor_row-1) {
					cursor_col=strlen(old_file[i]);
					new_file[i] = malloc(strlen(old_file[i])+strlen(old_file[i+1])+1);
					memcpy(new_file[i],old_file[i],strlen(old_file[i]));
					memcpy(&new_file[i][strlen(old_file[i])],old_file[i+1],1+strlen(old_file[i+1]));
					free(old_file[i]);
					free(old_file[i+1]);
				} else {
					new_file[i] = old_file[i+1];
				}
			}

			new_file[nlines-1] = 0;
			nlines--;
			cursor_row--;
			free(old_file);
			file = new_file;
			for (int i=cursor_row;i<nlines;i++) {
				refresh_line(i);
			}
			term_move_cursor(3+nlines, 1);
			printf("\033[2K");

			update_cursor();
		}
	} else {
		char* bak = file[cursor_row];
		char* new = malloc(strlen(bak));
		memcpy(new, bak, cursor_col-1);
		memcpy(&new[cursor_col-1], &bak[cursor_col], 1+strlen(bak)-cursor_col);

		file[cursor_row] = new;
		cursor_col--;
		free(bak);
		refresh_line(cursor_row);
		update_cursor();
	}
}

void editor_move(char dir) {
	if (dir == 'A') { // up
		cursor_row--;
	} else if (dir == 'B') { // down
		cursor_row++;
	} else if (dir == 'C') { // right
		cursor_col++;
	} else if (dir == 'D') { // left
		cursor_col--;
	}

	if (cursor_row >= nlines) {
		cursor_row = nlines - 1;
	}

	if (cursor_row < 0) {
		cursor_row = 0;
	}

	if (cursor_col < 0) {
		cursor_col = 0;
	}

	if (cursor_col > strlen(file[cursor_row])) {
		cursor_col = strlen(file[cursor_row]);
	}

	update_cursor();
}

void editor_putc(char c) {
	if (c == '\r') {
		char** old_file = file;
		char** new_file = malloc(sizeof(char*)*(nlines+2));
		for (int i=0;i<nlines+1;i++) {
			if (i < cursor_row) {
				new_file[i] = old_file[i];
			} else if (i == cursor_row) {
				new_file[i] = malloc(cursor_col+1);
				memcpy(new_file[i],old_file[i],cursor_col);
				new_file[i][cursor_col] = 0;
			} else if (i == cursor_row+1) {
				new_file[i] = malloc(strlen(old_file[i-1])-cursor_col+1);
				memcpy(new_file[i],&old_file[i-1][cursor_col],strlen(old_file[i-1])-cursor_col);
				new_file[i][strlen(old_file[i-1])-cursor_col] = 0;
				free(old_file[i-1]);
			} else {
				new_file[i] = old_file[i-1];
			}
		}

		new_file[nlines+1] = 0;
		nlines++;
		cursor_row++;
		cursor_col=0;
		free(old_file);
		file = new_file;
		for (int i=cursor_row-1;i<nlines;i++) {
			refresh_line(i);
		}
		update_cursor();
	} else {
		char* bak = file[cursor_row];
		char* new = malloc(strlen(bak)+2);
		memcpy(new, bak, cursor_col);
		new[cursor_col] = c;
		memcpy(&new[cursor_col+1], &bak[cursor_col], 1+strlen(bak)-cursor_col);
		file[cursor_row] = new;
		cursor_col++;
		free(bak);
		refresh_line(cursor_row);
		update_cursor();
	}
}
