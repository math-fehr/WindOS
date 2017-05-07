#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"
#include "errno.h"
#include "time.h"
#include "unistd.h"
#include "sys/wait.h"

extern int argc;
extern char** argv;
extern char** envp;

int main () {
	int pid = fork();
	if (pid != 0) {
		int status;
		while(1) {
			pid_t res = wait(&status);
			printf("[INIT] Process %d exited with status %d.\n", res, status);
		}
	} else {
		char* argv[] = {"/bin/wesh", NULL};
		execve("/bin/wesh", argv, NULL);
	}
}
