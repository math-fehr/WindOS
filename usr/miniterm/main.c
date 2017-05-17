#include "../../include/syscalls.h"
#include "../../include/termfeatures.h"
#include "../../include/loadfont.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

unsigned vspace(int fd, unsigned size, unsigned char color) {
    char line[3*1024];
    for (unsigned i=0;i<1024;i++) {
        line[3*i] = color;
        line[3*i+1] = color;
        line[3*i+2] = color;
    }
    for (unsigned c=0;c<size;c++) {
        _write(fd, line, 3*1024);
    }
    return size;
}


unsigned hspace(unsigned char* buffer, int h, int size, int offset, unsigned char color) {
    for(int j = 0; j<size; j++) {
        for(int i= 0; i<h; i++) {
            buffer[3*(i*1024+j+offset)] = color;
            buffer[3*(i*1024+j+offset)+1] = color;
            buffer[3*(i*1024+j+offset)+2] = color;
        }
    }
    return size;
}


char drawcharacter(unsigned char* buffer, unsigned char* font, int h, int w, int offset, char character, unsigned char ctext, unsigned char cback) {
    unsigned pos = h*w*character*3;
    unsigned char temp;
    for(int i = 0; i<h; i++) {
        for(int j = 0; j<w; j++) {
            if(font[3*(pos + i*w + j)]) {
                temp = ctext;
            }
            else {
                temp = cback;
            }
            buffer[(offset + i*1024 + j)*3] = temp;
            buffer[(offset + i*1024 + j)*3+1] = temp;
            buffer[(offset + i*1024 + j)*3+2] = temp;
        }
    }
    return w;
}


unsigned line(char* text, int fd, unsigned char* font, int h, int w, unsigned char ctext,unsigned char cback) {
    unsigned drawnlines = 0;
    drawnlines += vspace(fd,10,25);
    unsigned drawncol = 0;

    unsigned char buff[3*1024*h];
    drawncol += hspace(buff,h,5,0,cback);
    while(*text) {
        drawncol += drawcharacter(buff,font,h,w,drawncol,*text++,ctext,cback);
        hspace(buff,h,5,drawncol,25);
    }
    hspace(buff,h,1024-drawncol,drawncol,cback);
    _write(fd,buff,3*1024*h);
    drawnlines += h;
    drawnlines += vspace(fd,10,25);
    return drawnlines;
}

int main() {
	printf("Starting miniterm\n");
    int fd = _openat(AT_FDCWD, "/dev/fb",O_RDWR);

    unsigned height;
    unsigned width;
    unsigned char* font = NULL;

    loadFont("/fonts/font.bmp",&height,&width,&font);

    unsigned character = 'O';
    unsigned char buff[3*1024*height];
    for(int i = 0; i<height; i++) {
        for(int j = 0; j<width; j++) {
            buff[3*(i*1024 + j)] = *(font + 3*(character*height*width + i*width + j));
            buff[3*(i*1024 + j)+1] = *(font + 3*(character*height*width + i*width + j)+1);
            buff[3*(i*1024 + j)+2] = *(font + 3*(character*height*width + i*width + j)+2);
        }
    }
    _write(fd,buff,3*1024*height);
    //line("H",fd,font,height,width,0,255);
    /* line("Hello World!",fd,font,height,width,125,255); */
    /* line("Hello World!",fd,font,height,width,125,255); */
    /* line("Hello World!",fd,font,height,width,125,255); */

	_close(fd);

}
