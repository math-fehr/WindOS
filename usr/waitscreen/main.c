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

void h_to_rgb(int h, int* r, int *g, int *b) {
	int j = h % 360;
	if (j < 60) {
		*r = 255;
		*g = j*4;
		*b = 0;
	} else if (j < 2*60) {
		*r = 255+(60-j)*4;
		*g = 255;
		*b = 0;
	} else if (j < 3*60) {
		*r = 0;
		*g = 255;
		*b = (j-2*60)*4;
	} else if (j < 4*60) {
		*r = 0;
		*g = 255+(3*60-j)*4;
		*b = 255;
	} else if (j < 5*60) {
		*r = (j-4*60)*4;
		*g = 0;
		*b = 255;
	} else {
		*r = 255;
		*g = 0;
		*b = 255 + (5*60-j)*4;
	}
}



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
    unsigned char* bmp 		 = malloc(3*width_frame*height_frame);
	unsigned char* bmp_color = malloc(3*width_frame*height_frame);

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
            bmp_color[((height_bmp - i)*width_bmp + j)*3] = r;
            bmp_color[((height_bmp - i)*width_bmp + j)*3 + 1] = g;
            bmp_color[((height_bmp - i)*width_bmp + j)*3 + 2] = b;
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

	int last_time = _time(NULL);
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
			for (int j = 0;j<width_bmp; j++) {
				if (bmp[((height_bmp - i)*width_bmp + j)*3] != 0) {
					int r,g,b;
					int x_mod = new_posx % 360;
					int y_mod = new_posy % 360;
					h_to_rgb((x_mod*x_mod+y_mod*y_mod)/360, &r, &g, &b);
		            bmp_color[((height_bmp - i)*width_bmp + j)*3] = r;
		            bmp_color[((height_bmp - i)*width_bmp + j)*3 + 1] = g;
		            bmp_color[((height_bmp - i)*width_bmp + j)*3 + 2] = b;
				}
			}
		}

        for(int i = 0; i<height_bmp; i++) {
         //   _write(fd,blackline,3*leftsize);
			_lseek(fd,3*leftsize,SEEK_CUR);
            _write(fd,bmp_color+3*width_bmp*i,3*width_bmp);
   			_lseek(fd,3*rightsize,SEEK_CUR);
           // _write(fd,blackline,3*rightsize);
        }

		while (_time(NULL) - last_time < 2000);
		last_time = _time(NULL);

        posy = new_posy;
        posx = new_posx;
    }

	return 0;
}
