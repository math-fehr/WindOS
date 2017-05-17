#include "../../include/syscalls.h"
#include <string.h>
#include <stdio.h>

int main() {
	int fd[2];
	pipe(fd);

	int read = fd[0];
	int write = fd[1];



	for (int i = 0; i < 10; i++) {
		_write(write, "bonjour\n", strlen("bonjour\n"));
	}

	char buf[1000];
	int n = _read(read, buf, 1000);
	buf[n] = 0;
	printf("=> %s\n", buf);
}
