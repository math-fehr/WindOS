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
			if (pid == res) {
				printf("[INIT] The shell is dead. Restarting..\n");
				main();
			}
		}
	} else {
		char* argv_wesh[] = {"/bin/wesh", NULL};
		char* envp_wesh[] = {"PATH=/bin", NULL};
		execve("/bin/wesh", argv_wesh, envp_wesh);
	}
}
