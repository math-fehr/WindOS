#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "../../include/dirent.h"
#include "../../include/syscalls.h"

extern int argc;
extern char** argv;
extern char** environ;


int delete(int fd, char* path, bool recursive) {
	struct stat fs;
	int dest = _openat(fd, path, O_RDONLY);
	if (dest > 0) {
		fstat(dest, &fs);

		if (S_ISDIR(fs.st_mode)) {
			if (recursive) {
				struct dirent entry;
				while(_getdents(dest, &entry) == 0) {
					if (strcmp(entry.d_name, ".") && strcmp(entry.d_name, "..")) {
						delete(dest, entry.d_name, true);
					}
				}

				_unlinkat(fd, path, 0);
				_close(dest);
			} else {
				errno = EISDIR;
				perror("rm");
			}
		} else {
			_unlinkat(fd, path, 0);
			_close(dest);
		}
	} else {
		perror("rm");
		return -1;
	}
	return 0;
}


int main() {
	char* params[18];
	for (int i=0;i<argc;i++) {
		params[i] = argv[i];
	}

	bool recursive = false;
	int opt;
	while ((opt = getopt(argc,params,"r")) != -1) {
		switch (opt) {
			case 'r':
				recursive = true;
				break;
			case '?':
				break;
			default:
				fprintf(stderr, "Usage: rm [-r] [path]");
		}
	}

	if (argv[optind] != NULL) {
		char* target = argv[optind];
		return delete(AT_FDCWD, target, recursive);
	} else {
		fprintf(stderr, "Usage: rm [-r] [path]");
		return -1;
	}
}
