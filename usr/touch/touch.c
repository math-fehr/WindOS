#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "../../include/syscalls.h"

extern int argc;
extern char** argv;

int main() {
	if (argc > 1) {
		int fd = _open(argv[1], O_CREAT);
		if (fd < 0) {
			perror("touch");
		}
		_close(fd);
		return 0;
	} else {
		printf("touch: no file specified.\n");
		return 1;
	}
}
