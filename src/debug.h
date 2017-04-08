#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "serial.h"

void kernel_printf(const char* fmt, ...);

void print_hex(int number, int nbytes);

int min(int a, int b);

// computes a^b
int ipow(int a, int b);
#endif
 
