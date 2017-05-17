#include "../../include/syscalls.h"
#include "../../include/termfeatures.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

int main() {
	printf("Starting miniterm\n");
	int fd = _openat(AT_FDCWD, "/dev/fb",O_RDWR);
	if (fd < 0) {
		printf("Failed opening framebuffer.\n");
		perror("miniterm");
		return 1;
	} else {
		char line[3*1024];
		for (int i=0;i<1024;i++) {
			line[3*i] = 25;
			line[3*i+1] = 25;
			line[3*i+2] = 25;
		}
		for (int c=0;c<768;c++) {
			_write(fd, line, 3*1024);
		}
	}
	_close(fd);

}
