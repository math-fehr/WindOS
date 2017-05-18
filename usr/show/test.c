#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
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

    int fd = _openat(AT_FDCWD, "/dev/fb", O_RDWR);

    if(f == NULL || fd < 0) {
        return -1;
    }

    _ioctl(fd,FB_OPEN,0);
    _ioctl(fd,FB_SHOW,0);
    _ioctl(fd,FB_BUFFERED,0);

    int height_frame = _ioctl(fd,FB_HEIGHT,0);
    int width_frame = _ioctl(fd,FB_WIDTH,0);


    //Basic info on BMP
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f);
	int width = *(int*)&info[18];
    int height = *(int*)&info[22];
	int off = *(int*)&info[10];

    //A row in the BMP
	int row_padded = (width*3 + 3) & (~3);
	unsigned char* data = malloc(row_padded);
	fread(data, sizeof(unsigned char), off-54, f);

    //The scale of the image
    int multiply = height_frame/height;
    if(width_frame/width < multiply) {
        multiply = width_frame/width;
    }

    //The black borders
    unsigned char buffer[3*width_frame];
    memset(buffer,0,3*width_frame);
    int leftsize = (width_frame - multiply*width)/2;
    int upsize = (height_frame - multiply*height)/2;
    int downsize = height_frame - upsize - multiply*height;

    printf("\n%d %d %d %d",height_frame,width_frame,height,width);

    //We write the up and down borders
    for(int i = 0; i<upsize; i++) {
        _write(fd,buffer,3*width_frame);
    }
    _lseek(fd,width_frame*3*(height_frame-downsize),SEEK_SET);
    for(int i = 0; i<downsize; i++) {
        _write(fd,buffer,3*width_frame);
    }


    for(int i = 0; i<height; i++) {
        if(i!=0) {
            _lseek(fd,-multiply*2*3*width_frame,SEEK_CUR);
        } else {
            _lseek(fd,-(multiply+downsize)*3*width_frame,SEEK_CUR);
        }
        fread(data,sizeof(unsigned char), row_padded,f);
        for(int j = 0; j<width; j++) {
            unsigned char r = data[3*j+2];
            unsigned char g = data[3*j+1];
            unsigned char b = data[3*j];
            for(int m = 0; m<multiply; m++) {
                buffer[(leftsize + j*multiply + m)*3] = r;
                buffer[(leftsize + j*multiply + m)*3 + 1] = g;
                buffer[(leftsize + j*multiply + m)*3 + 2] = b;
            }
        }
        for(int m = 0; m<multiply; m++) {
            _write(fd,buffer,3*width_frame);
        }
    }

    /*for (int y=0;y<height;y++) {
		fread(data, sizeof(unsigned char), row_padded, f);
		for (int x=0;x<width;x++) {
			printf("\033[%d;%dH ",height-y,x);
			unsigned char r = data[3*x+2];
			unsigned char g = data[3*x+1];
			unsigned char b = data[3*x];
			term_set_bg(r,g,b,false);
		}
    }*/

	return 0;
}
