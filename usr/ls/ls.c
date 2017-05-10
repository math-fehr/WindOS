#include "stdio.h"
#include "stdlib.h"
#include "../../include/dirent.h"

extern int argc;
extern char** argv;
extern char** environ;

int main() {
	int fd;
	
	if (argc >= 2) {
		fd = _open(argv[1],0);
	} else {
		fd = _open(".",0);
	}

	if (fd < 0) {
		perror("ls");
		return 1;
	} else {
		struct dirent* entry = malloc(sizeof(struct dirent));
		while(_getdents(fd,entry) == 0) {
			printf("%s ", entry->d_name);
		};
		printf("\n");
		return 0;
	}

}
