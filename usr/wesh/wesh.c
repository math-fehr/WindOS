#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"
#include "errno.h"
#include "time.h"
#include "unistd.h"
#include "sys/wait.h"
#include "string.h"

#include "../../include/termfeatures.h"

extern int argc;
extern char** argv;
extern char** environ;

int exec_blocking(char* command, char* params[]) {
	int r = _fork();
	if (r == 0) {
		execvp(command, params);
		perror(command);
		_exit(1);
	} else {
		return wait(NULL);
	}
}

#define MAX_HISTORY 30
#define MAX_BUF 	256

char wesh_history[MAX_HISTORY][MAX_BUF];

int hist_top; // first free slot.
int hist_bot; // first occupied slot.

void wesh_readline(char* buffer) {
	int pos = 0;
	int hist_pos = hist_top;
	char* current_buffer = buffer;
	buffer[0] = 0;
	while(1) {
		char c = getc(stdin);
		if (c == 0x7F) { // delete
			if (pos > 0) {
				current_buffer[pos--] = 0;
				printf("\033[D \033[D");
				fflush(stdout);
			}
		} else if (c == 0x1B) { // escape code
			c = getc(stdin); // normally [
			c = getc(stdin);
			if (hist_bot != hist_top) {
				char* new_entry;

				if (c == 'A') { // ^
					if (hist_pos != hist_bot) {
						hist_pos--;
						if (hist_pos < 0) {
							hist_pos = MAX_HISTORY-1;
						}
						new_entry = wesh_history[hist_pos];
					}
				} else if (c == 'B') { // v
					if (hist_pos != hist_top) {
						hist_pos++;

						if (hist_pos == MAX_HISTORY) {
							hist_pos = 0;
						}

						if (hist_pos == hist_top) {
							new_entry = buffer;
						} else {
							new_entry = wesh_history[hist_pos];
						}
					}
				}
				for (int i=0;i<pos;i++) {
					printf("\033[D \033[D");
				}
				printf("%s", new_entry);
				current_buffer = new_entry;
				pos = strlen(current_buffer);
			}

		} else { // basic char
			current_buffer[pos] = c;
			pos++;
			current_buffer[pos] = 0;
			printf("%c", c);
			fflush(stdout);

			if (c == '\r') {
				current_buffer[pos-1] = 0;
				strcpy(buffer, current_buffer);
				return;
			}
		}
	}
}

int main() {
	printf("WESH is an Experimental SHell.\n");
	hist_top = 0;
	hist_bot = 0;

	while(1) {
		char buf[1500];
		getcwd(buf, 1500);
		printf("%s$ ", buf);
		term_raw_enable(true);
		wesh_readline(buf);
		term_raw_enable(false);

		strcpy(wesh_history[hist_top], buf);

		hist_top++;
		if (hist_top == MAX_HISTORY)
			hist_top = 0;

		char* token = strtok(buf," ");
		char* params[32];
		params[0] = token;
		if (strcmp(buf, "exit") == 0) {
			return 0;
		} else if (strcmp(buf, "cd") == 0) {
			token = strtok(NULL," ");
			if (token == NULL)
				_chdir("/");
			else
				_chdir(token);
		} else {
			int i = 0;
			while(token != NULL) {
				i++;
				token = strtok(NULL," ");
				params[i] = token;
			}
			exec_blocking(params[0], params);
		}
	}
  	return 0;
}
