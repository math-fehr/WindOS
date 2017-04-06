#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "serial.h"

void kernel_error(char* source, char* message);
void kernel_info(char* source, char* message);

void print_hex(int number, int nbytes);

#endif
