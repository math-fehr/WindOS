#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "../../include/termfeatures.h"
#include "../../include/syscalls.h"

extern int argc;
extern char** argv;
extern char** environ;

int main() {
	/*printf("\033[2J");
	printf("\033[10;30H");
	printf("\033[30;47m     BONJOUR    \033[39;49m\n");

	for (volatile int i=0;i<10000000;i++) {}
	printf("\033[2J");
	printf("\033[1;1H");
*/

	FILE* f;
	if (argc == 2) {
		f = fopen(argv[1], "rb");
	} else {
		f = fopen("/surprise.bmp", "rb");
	}
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f);

	int width = *(int*)&info[18];
    int height = *(int*)&info[22];
	int off = *(int*)&info[14];

	printf("w: %d\nh: %d\n o: %d\n", width, height, off);

	int row_padded = (width*3 + 3) & (~3);
	unsigned char* data = malloc(row_padded);
	fread(data, sizeof(unsigned char), off-43, f);

	for (int y=0;y<height;y++) {
		fread(data, sizeof(unsigned char), row_padded, f);
		for (int x=0;x<width;x++) {
			printf("\033[%d;%dH ",height-y,x);
			unsigned char r = data[3*x+2];
			unsigned char g = data[3*x+1];
			unsigned char b = data[3*x];
			term_set_bg(r,g,b,false);
		}
	}

	return 0;
}
