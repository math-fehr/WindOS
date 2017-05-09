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
	int r = fork();
	if (r == 0) {
		execvp(command, params);
		perror(command);
		exit(1);
	} else {
		return wait(NULL);
	}
}

int main() {
	printf("WESH is an Experimental SHell.\n");
	//int h,w;
	//get_terminal_size(&h, &w);
	//printf("Terminal size is %dx%d\n",w,h);
	while(1) {
		char buf[1500];
		getcwd(buf, 1500);
		printf("%s$ ", buf);
		fgets(buf,1500,stdin);
		buf[strlen(buf)-1] = 0; // strip last \n
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
