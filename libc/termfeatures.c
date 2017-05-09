#include "../include/termfeatures.h"

void get_terminal_size(int* height, int* width) {
	printf("\033[999;999H"); // move cursor far away
	fflush(stdout);
	printf("\033[6n"); // Device Status Report
	fflush(stdout);
	char buf[10];
	scanf("%s", buf);
}
