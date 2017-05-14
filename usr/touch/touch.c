#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int argc;
char* argv[];

int main() {
	if (argc > 1) {
		_close(_open(argv[1], O_CREAT));
		return 0;
	} else {
		printf("touch: no file specified.\n");
		return 1;
	}
}
