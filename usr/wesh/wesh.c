#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"
#include "errno.h"
#include "time.h"
#include "unistd.h"
#include "sys/wait.h"
#include "string.h"

extern int argc;
extern char** argv;
extern char** environ;

int exec_blocking(char* command) {
	int r = fork();
	if (r == 0) {
		char* params[] = {command, NULL};
		execvp(command, params);
		perror("execvp");
		exit(1);
	} else {
		return wait(NULL);
	}
}

int main() {
	while(1) {
		char buf[1500];
		exec_blocking("pwd");
		printf("$ ");
		scanf("%s",buf);
		if(strcmp(buf,"exit") == 0) {
			break;
		}
	}
  	return 0;
}
