#include "debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * The number of sources
 */
#define N_SOURCES 12

/**
 * The sources of log
 */
const char sources[N_SOURCES][9]= { "KERNEL", "SERIAL", "WESH",
                                    "TIMER" , "IRQ"   , "VFS" ,
                                    "EXT2"  , "GPIO"  , "PROCESS",
                                    "MEMORY", "SDCARD", "SYSCALL"};

/**
 * The value used to know when to output log
 */
const int enable_source[N_SOURCES] = { 5,10,10,
                                       10,5,5,
									   5,5,5,
									   5,10,5};


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


/**
 * Used in function kdebug.
 * Act the same as kernel_printf, but when the va_list is already set up
 */
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
