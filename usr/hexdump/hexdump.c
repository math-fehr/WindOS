#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "../../include/syscalls.h"

extern int argc;
extern char** argv;
extern char** environ;


int main() {
	int col = 80;
	char buffer[1025];
	for (int i=1;i<argc;i++) {
		int fd = _open(argv[i], O_RDONLY);
		if (fd >= 0) {
			int n;
			printf("%s:", argv[i]);
			int addr = 0;
			while((n = _read(fd, buffer, 1024)) > 0) {
				for (int i=0;i<n;i++) {
					if (addr % 16 == 0) {
						printf("\n");
						printf("%#07x", addr);
					}
					if (addr % 2 == 0) {
						printf(" ");
					}
					printf("%02x", buffer[i]);
					addr++;
				}
			}
			printf("\n");
			_close(fd);
		} else {
			sprintf(buffer, "cat: %s", argv[i]);
			perror(buffer);
		}
	}
}
