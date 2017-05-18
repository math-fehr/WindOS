#include "../../include/syscalls.h"

#include <stdio.h>

int main() {
	int x1 = _fork();
	int x2 = _fork();

	printf("%d %d\n", x1, x2);
	printf("%d %d\n", x1, x2);
	printf("%d %d\n", x1, x2);
	printf("%d %d\n", x1, x2);
	return 0;
}
