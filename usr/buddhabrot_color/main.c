#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "../../include/termfeatures.h"
#include "../../include/syscalls.h"

unsigned window_h;
unsigned window_w;



int main() {

    int fd = _openat(AT_FDCWD, "/dev/fb", O_RDWR);
    if(fd < 0) {
        return -1;
    }
    printf("Ici");
    _ioctl(fd,FB_OPEN,0);
    _ioctl(fd,FB_SHOW,0);
    _ioctl(fd,FB_BUFFERED,0);

    window_h = _ioctl(fd,FB_HEIGHT,0);
    window_w = _ioctl(fd,FB_WIDTH,0);

    unsigned char* img = calloc(window_w*window_h*3,1);

    float x1 = -2.1;
    float x2 = 0.6;
    float y1 = -1.2;
    float y2 = 1.2;
    float zoom_x = window_w / (x2-x1);
    float zoom_y = window_h / (y2-y1);
    long int const iteration_max_r = 7;
    long int const iteration_max_g = 10;
    long int const iteration_max_b = 13;

    unsigned temp_r[iteration_max_r*2];
    unsigned temp_g[iteration_max_r*2];
    unsigned temp_b[iteration_max_r*2];

    for(unsigned x = 0; x< window_w; x++) {
      //  printf("%d\n", x);
        for(unsigned y = 0; y<window_h; y++) {
            float c_r = x / zoom_x + x1;
            float c_i = y / zoom_y + y1;
            float z_r = 0;
            float z_i = 0;
            int i = 0;
            unsigned n_temp = 0;

            do {
                float tmp = z_r;
                z_r = z_r*z_r - z_i*z_i + c_r;
                z_i = 2*z_i*tmp + c_i;
                i++;
                temp_r[n_temp*2] = (z_r-x1)*zoom_x;
                temp_r[n_temp*2+1] = (z_i-y1)*zoom_y;
                temp_g[n_temp*2] = (z_r-x1)*zoom_x;
                temp_g[n_temp*2+1] = (z_i-y1)*zoom_y;
                temp_b[n_temp*2] = (z_r-x1)*zoom_x;
                temp_b[n_temp*2+1] = (z_i-y1)*zoom_y;
                n_temp++;
            } while(z_r*z_r+z_i*z_i < 4 && i < iteration_max_r);

            do {
                float tmp = z_r;
                z_r = z_r*z_r - z_i*z_i + c_r;
                z_i = 2*z_i*tmp + c_i;
                i++;
                temp_g[n_temp*2] = (z_r-x1)*zoom_x;
                temp_g[n_temp*2+1] = (z_i-y1)*zoom_y;
                temp_b[n_temp*2] = (z_r-x1)*zoom_x;
                temp_b[n_temp*2+1] = (z_i-y1)*zoom_y;

                n_temp++;
            } while(z_r*z_r+z_i*z_i < 4 && i < iteration_max_g);

            do {
                float tmp = z_r;
                z_r = z_r*z_r - z_i*z_i + c_r;
                z_i = 2*z_i*tmp + c_i;
                i++;
                temp_b[n_temp*2] = (z_r-x1)*zoom_x;
                temp_b[n_temp*2+1] = (z_i-y1)*zoom_y;
                n_temp++;
            } while(z_r*z_r+z_i*z_i < 4 && i < iteration_max_b);

            for(int j = 0; j<iteration_max_r; j++) {
                if(temp_r[j*2] < window_w && temp_r[j*2+1] < window_h && img[(temp_r[j*2] + window_w*temp_r[j*2+1])*3] < 100) {
                    img[(temp_r[j*2] + window_w*temp_r[j*2+1])*3]+=8;
                }
                else if(temp_r[j*2] < window_w && temp_r[j*2+1] < window_h && img[(temp_r[j*2] + window_w*temp_r[j*2+1])*3] < 200) {
                    img[(temp_r[j*2] + window_w*temp_r[j*2+1])*3]+=4;
                }
            }


            for(int j = 0; j<iteration_max_g; j++) {
                if(temp_g[j*2] < window_w && temp_g[j*2+1] < window_h && img[(temp_g[j*2] + window_w*temp_g[j*2+1])*3+1] < 100) {
                    img[(temp_g[j*2] + window_w*temp_g[j*2+1])*3+1]+=8;
                }
                else if(temp_g[j*2] < window_w && temp_g[j*2+1] < window_h && img[(temp_g[j*2] + window_w*temp_g[j*2+1])*3+1] < 200) {
                    img[(temp_g[j*2] + window_w*temp_g[j*2+1])*3+1]+=4;
                }
            }


            for(int j = 0; j<iteration_max_b; j++) {
                if(temp_b[j*2] < window_w && temp_b[j*2+1] < window_h && img[(temp_b[j*2] + window_w*temp_b[j*2+1])*3+2] < 100) {
                    img[(temp_b[j*2] + window_w*temp_b[j*2+1])*3+2]+=4;
                }
                if(temp_b[j*2] < window_w && temp_b[j*2+1] < window_h && img[(temp_b[j*2] + window_w*temp_b[j*2+1])*3+2] < 200) {
                    img[(temp_b[j*2] + window_w*temp_b[j*2+1])*3+2]+=8;
                }
            }

        }

        if(!(x%5)) {
            _write(fd,img,3*window_w*window_h);
            _lseek(fd,0,SEEK_SET);
        }
    }
    _write(fd,img,3*window_w*window_h);

	fgetc(stdin);
	_ioctl(fd, FB_CLOSE, 0);
    return 0;
}
