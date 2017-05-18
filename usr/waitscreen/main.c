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
	FILE* f;
	if (argc == 2) {
		f = fopen(argv[1], "rb");
	} else {
		f = fopen("/logo.bmp", "rb");
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
    unsigned char* bmp = malloc(3*width_frame*height_frame);

    //Basic info on BMP
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f);
	int width_bmp = *(int*)&info[18];
    int height_bmp = *(int*)&info[22];
	int off = *(int*)&info[10];

    //A row in the BMP
	int row_padded = (width_bmp*3 + 3) & (~3);
	unsigned char* data = malloc(row_padded);
	fread(data, sizeof(unsigned char), off-54, f);

    for(int i = 0; i<height_bmp; i++) {
    fread(data,sizeof(unsigned char), row_padded,f);
        for(int j = 0; j<width_bmp; j++) {
            unsigned char r = data[3*j+2];
            unsigned char g = data[3*j+1];
            unsigned char b = data[3*j];
            bmp[((height_bmp - i)*width_bmp + j)*3] = r;
            bmp[((height_bmp - i)*width_bmp + j)*3 + 1] = g;
            bmp[((height_bmp - i)*width_bmp + j)*3 + 2] = b;
            }
        }

    char blackline[width_frame*3];
    for(int i = 0; i<width_frame*3; i++) {
        blackline[i] = 0;
    }
    for(int i = 0; i<height_frame; i++) {
        _write(fd,blackline,3*width_frame);
    }


    int posx = 10;
    int posy = 10;
    int base_speed_x = 6;
    int base_speed_y = 6;
    int speedx = base_speed_x;
    int speedy = base_speed_y;

    while(1) {
        int new_posx = posx + speedx;
        int new_posy = posy + speedy;

        if(new_posx < 0) {
            new_posx += 2*base_speed_x;
            speedx = base_speed_x;
        }
        if(new_posy < 0) {
            new_posy += base_speed_y;
            speedy = base_speed_y;
        }
        if(new_posx >= width_frame - width_bmp) {
            new_posx -= 2*base_speed_x;
            speedx = -base_speed_x;
        }
        if(new_posy >= height_frame - height_bmp) {
            new_posy -= 2*base_speed_y;
            speedy = -base_speed_y;
        }

        _lseek(fd,new_posy*3*width_frame,SEEK_SET);
        int leftsize = new_posx;
        int rightsize = width_frame - new_posx - width_bmp;

        for(int i = 0; i<height_bmp; i++) {
            _write(fd,blackline,3*leftsize);
            _write(fd,bmp+3*width_bmp*i,3*width_bmp);
            _write(fd,blackline,3*rightsize);
        }

        posy = new_posy;
        posx = new_posx;
    }

	return 0;
}
