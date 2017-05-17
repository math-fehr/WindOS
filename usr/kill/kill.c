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

int main() {
	if (argc > 2) {
		int signal = atoi(argv[1]);
		int pid = atoi(argv[2]);
		if (_kill(pid,signal) == -1) {
			perror("kill");
		}
	} else {
		printf("kill: usage: kill [signal] [pid]\n");
	}
}
