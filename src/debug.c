#include "debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define N_SOURCES 8

const char sources[N_SOURCES][9]= { "KERNEL", "SERIAL", "WESH",
                                    "TIMER" , "IRQ"   , "VFS" ,
                                    "EXT2"  , "GPIO"           };

const int enable_source[N_SOURCES] = { 10,10,10,
                                       10,10,10,
                                       10,10};

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

void vkernel_printf(const char* fmt, va_list args) {
  int size = vsnprintf(NULL, 0, fmt, args);
  char* buffer = (char*) malloc(size+1);
  vsnprintf(buffer, size+1, fmt, args);
  buffer[size] = 0;
  serial_write(buffer);
  free(buffer);
}

void kdebug(int from, int level, const char* fmt, ...) {
  if (from < 0 || from >= N_SOURCES) return;
  if (enable_source[from] > level) return;
  kernel_printf("[%s][%d] ", sources[from], level);

  va_list args;
  va_start(args, fmt);
  vkernel_printf(fmt, args);
  va_end(args);
}

int min(int a, int b) {
  return a < b ? a : b;
}

int max(int a, int b) {
  return a > b ? a : b;
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
