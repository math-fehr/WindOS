#include "../../include/syscalls.h"

#include <stdio.h>

int main() {
	int x1 = _fork();
	int x2 = _fork();
	int x3 = _fork();
	int x4 = _fork();

	printf("%d %d %d %d\n", x1, x2, x3, x4);
	return 0;
}
