#include "debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void kernel_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int size = vsnprintf(NULL, 0, fmt, args);
  char* buffer = (char*) malloc(size+1);
  vsnprintf(buffer, size+1, fmt, args);
  va_end(args);
  buffer[size] = 0;
  serial_write(buffer);
  free(buffer);
}

void print_hex(int number, int nbytes) {
  serial_write("  0x");
  for (int i=0;i<2*nbytes;i++) {
    char hx = (number >> (4*(2*nbytes-1-i))) & 15;
    if (hx < 10) {
      serial_putc(48+hx); 
    } else {
      serial_putc(65+hx-10);
    }
  }
  serial_newline();
}

int min(int a, int b) {
  return a < b ? a : b;
}

// computes a^b
int ipow(int a, int b) {
  int result = 1;

  while (b){
    if (b&1){
      result *= a;
    }
    b >>=1 ;
    a *= a;
  }
  return result;
}
