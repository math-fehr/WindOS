#include "../include/loadfont.h"
#include "../include/syscalls.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * width_ptr and height_ptr should be allocated, font musn't
 */
bool loadFont(char* path_font, unsigned* width_ptr, unsigned* height_ptr, unsigned char** font) {
    FILE* f = fopen(path_font,"rb");

    if(f == NULL || width_ptr == NULL || height_ptr == NULL) {
        return false;
    }

    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f);

    unsigned img_width = *(int*)&info[18];
    unsigned img_height = *(int*)&info[22];

    unsigned width = img_width/16;
    unsigned height= img_height/16;

    *height_ptr = height;
    *width_ptr = width;

	int off = *((int*)&info[10]);

    *font = malloc(sizeof(unsigned char) * (img_width) * (img_height) * 3);

    int row_padded = (img_width*3 + 3) & (~3);
	unsigned char* data = malloc(row_padded);
	fread(data, sizeof(unsigned char), off-54, f);

    unsigned char temp;
	for (unsigned y=0;y<img_height;y++) {
		fread(data, sizeof(unsigned char), row_padded, f);
		for (unsigned x=0;x<img_width;x++) {
			unsigned char r = data[3*x+2];
			unsigned char g = data[3*x+1];
			unsigned char b = data[3*x];

            if(r != 0 || g != 0 || b != 0) {
                temp = 255;
            } else {
                temp = 0;
            }

            unsigned char_x = x/width;
            unsigned char_y = (img_height-y)/height;
            unsigned character = char_y * 16 + char_x;
            unsigned pos_x = x % width;
            unsigned pos_y = (img_height-y) % height;

            *(*font + 3*(character*width*height + pos_y*width + pos_x )) = temp;
            *(*font + 3*(character*width*height + pos_y*width + pos_x ) +1) = temp;
            *(*font + 3*(character*width*height + pos_y*width + pos_x ) +2) = temp;
		}
	}

    return true;
}
