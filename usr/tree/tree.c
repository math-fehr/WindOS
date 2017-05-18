#include "stdio.h"
#include "stdlib.h"
#include "../../include/dirent.h"
#include "../../include/termfeatures.h"
#include "../../include/syscalls.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

extern int argc;
extern char** argv;

int main() {
	char* target = ".";

	if (argc > 1) {
		target = argv[1];
	}

	int fd = _open(target,O_RDONLY);

	if (fd < 0) {
		perror("ls");
		return 1;
	} else {

	}
}
