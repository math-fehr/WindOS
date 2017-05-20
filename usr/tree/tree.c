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

int explore(int fd, int depth, char* entry) {
	for (int i=0;i<depth-1;i++) {
		printf("| ");
	}
	if (depth > 0) {
		printf("|-");
	}
	printf("%s\n", entry);
	int result;
	struct dirent st_entry;
	while((result = _getdents(fd,&st_entry)) == 0) {
		if (st_entry.d_name[0] != '.') {
			int fd_exp = _openat(fd, st_entry.d_name, O_RDONLY);
			explore(fd_exp, depth+1, st_entry.d_name);
			_close(fd_exp);
		}
	}
}



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
		explore(fd, 0, target);
		_close(fd);
	}
}
