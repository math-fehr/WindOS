#include "stdio.h"
#include "stdlib.h"
#include "../../include/dirent.h"
#include "../../include/termfeatures.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

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
			if (S_ISDIR(entry->d_type)) {
				term_set_fg(51, 51, 255, false);
			} 
			printf("%s ", entry->d_name);
			term_default_colors();
		};
		printf("\n");
		return 0;
	}

}
