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
	printf("Init started\n");
	int pid = _fork();

	if (pid != 0) { // The father is the reaper.
		int status;
		while(1) {
			pid_t res = _wait(&status);
			if (WIFEXITED(status)) {
				printf("[INIT] Process %d exited with status %d.\n", res, WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				printf("[INIT] Process %d killed by signal %d.\n", res, WTERMSIG(status));
			}
			/*if (pid == res) {
				printf("[INIT] The shell is dead. Long live the shell..\n");
				main();
			}*/
		}
	} else { // process 1
		printf("Forked\n");
		char* argv_wesh[] = {"/bin/wesh",NULL};
		char* envp_wesh[] = {"PATH=/bin/",NULL};
		execve("/bin/wesh", argv_wesh, envp_wesh);
	}
}
