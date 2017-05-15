#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../include/syscalls.h"

int main() {
	char buf[255];
	getcwd(buf, 255);
	printf("%s\n", buf);
}
