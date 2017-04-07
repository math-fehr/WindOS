//http://www.valvers.com/open-software/raspberry-pi/step02-bare-metal-programming-in-c-pt2/

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"


intptr_t _sbrk(int incr) {
  static char* heap_end = 0;
  char* prev_heap_end;
  if(heap_end == 0) // first initialization of heap.
    heap_end = (char*) (16*1024*1024); // TODO: not hardcode that and maybe place it somewhere else.

  prev_heap_end = heap_end;
  heap_end += incr;
  return (intptr_t)prev_heap_end;
}
