#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"
#include "errno.h"
#include "time.h"
#include "unistd.h"
#include "sys/wait.h"
#include "string.h"
#include "fcntl.h"


#include "../../include/termfeatures.h"
#include "../../include/syscalls.h"
#include "../../include/dirent.h"

extern int argc;
extern char** argv;

int exec_blocking(char* command, char* params[], char* input, char* output) {
	int r = _fork();
	int fd1 = dup(1);
	if (r == 0) {
		if (input != NULL) {
			int fd_in = _open(input, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd_in < 0) {
				perror(command);
			} else {
				dup2(fd_in, 0);
			}
		}

		if (output != NULL) {
			int fd_out = _open(output, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd_out < 0) {
				perror(command);
			} else {
				dup2(fd_out, 1);
			}
		}

		execvp(command, params);
		dup2(fd1, 1);
		perror(command);
		_exit(1);
	} else {
		return _waitpid(r, NULL, 0);
	}
}

#define MAX_HISTORY 30
#define MAX_BUF 	256

char wesh_history[MAX_HISTORY][MAX_BUF];

int hist_top; // first free slot.
int hist_bot; // first occupied slot.


void autocomplete(char* currentBuffer, int* pos) {
    char* last = strrchr(currentBuffer, ' ');

    if(last == NULL) {
        last = currentBuffer;
    }
    char* beg_file = basename(last);
    char* folder = dirname(last);

    unsigned size_beg_file = strlen(beg_file);

    int fd = _open(folder,O_RDONLY);

    char* best_prefix = beg_file;
    int size_best_prefix = size_beg_file;
    bool found_one = false;

    struct dirent* entry = malloc(sizeof(struct dirent));
    while(_getdents(fd,entry) == 0) {
        if(strlen(entry->d_name) > size_beg_file && strncmp(entry->d_name,beg_file,size_beg_file) == 0) {
            if(!found_one) {
                found_one = true;
                strcpy(best_prefix,entry->d_name);
                size_best_prefix = strlen(entry->d_name);
            } else {
                int i = size_beg_file;
                while(entry->d_name[i] != 0 && entry->d_name[i] == best_prefix[i] && i<size_best_prefix) {
                    i++;
                }
                if(i != size_best_prefix) {
                    size_best_prefix = i;
                    strncpy(best_prefix,best_prefix,i);
                }
            }
        }
    }
    if(found_one) {
        printf("%s", best_prefix+size_beg_file);
        *pos += size_best_prefix-size_beg_file;
    }
    return;
}


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

		} else if(c == 0x9) {
            autocomplete(current_buffer,&pos);
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

        char* base_command = strtok(buf," ");
        char* input = NULL;
        char* output = NULL;
		char* params[32];
		params[0] = base_command;

        char* temp;
        bool next_input = false;
        bool next_output = false;

		bool failed = false;

		int cur_param = 1;

        while(((temp = strtok(NULL," ")) != NULL )&& !failed) {
			if (strcmp(temp, "<") == 0) {
				if (!next_output) {
					next_input = true;
				} else {
					failed = true;
				}
			} else if (strcmp(temp, ">") == 0) {
				if (!next_input) {
					next_output = true;
				} else {
					failed = true;
				}
			} else {
				if (next_input) {
					input = temp;
					next_input = false;
				} else if (next_output) {
					output = temp;
					next_output = false;
				} else {
					params[cur_param] = temp;
					cur_param++;
				}
			}
        }

		if (strcmp(base_command, "exit") == 0) {
			return 0;
		} else if (strcmp(base_command, "cd") == 0) {
			if (params[1] == NULL) {
				_chdir("/");
			}
			else {
				int res = _chdir(params[1]);
				if (res == -1) {
					perror("cd");
				}
			}
		} else {
			params[cur_param] = 0;
			exec_blocking(base_command, params, input, output);
	        input = NULL;
	        output = NULL;
		}
	}
  	return 0;
}
