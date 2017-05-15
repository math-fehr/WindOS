#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"
#include "errno.h"
#include "time.h"
#include "unistd.h"
#include "sys/wait.h"

#include "../../include/syscalls.h"

extern int argc;
extern char** argv;
extern char** envp;

int main () {
	int pid = _fork();

	if (pid != 0) {
		int status;
		while(1) {
			pid_t res = _wait(&status);
			//printf("[INIT] Process %d exited with status %d.\n", res, status);
			if (pid == res) {
				printf("[INIT] The shell is dead. Long live the shell..\n");
				main();
			}
		}
	} else {
		pid = _fork();
		if (pid == 0) {
			while (1) {}
		} else {
			char* argv_wesh[] = {"/bin/wesh", NULL};
			char* envp_wesh[] = {"PATH=/bin", NULL};
			execve("/bin/wesh", argv_wesh, envp_wesh);
		}
	}
}
