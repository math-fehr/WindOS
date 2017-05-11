#ifndef PICO_H
#define PICO_H

#include <string.h>
#include "stdio.h"
#include "stdlib.h"

#include "../../include/termfeatures.h"

void topbar_draw();
void editor_delete();
void editor_move(char dir);
void editor_putc(char c);
void editor_draw();


#endif
