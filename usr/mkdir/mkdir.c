#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "../../include/syscalls.h"

int argc;
char** argv;

int main() {
	if (argc > 1) {
		if (_mknodat(AT_FDCWD, argv[1], S_IFDIR, 0) == 0) {
			return 0;
		}
		perror("mkdir");
		return -1;
	} else {
		printf("touch: no file specified.\n");
		return 1;
	}
}
