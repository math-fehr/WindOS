//http://www.valvers.com/open-software/raspberry-pi/step02-bare-metal-programming-in-c-pt2/

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

#include "memalloc.h"
#include "mmu.h"


//#define HEAP_BASE 0xc0000000
#define HEAP_BASE 0xc0000000
#define KERNEL_TTB_ADDRESS 0xf0004000

extern int __kernel_phy_end;

uintptr_t _sbrk(int incr) {
  static uintptr_t heap_end = 0;
  static uint32_t allocated_pages = 0;
  uintptr_t prev_heap_end;
  if(heap_end == 0) { // first initialization of heap.
    heap_end = (uintptr_t) HEAP_BASE;
    allocated_pages = 1;

    mmu_add_section(KERNEL_TTB_ADDRESS,
                    HEAP_BASE,
                    (uintptr_t)&__kernel_phy_end + PAGE_SECTION - 1,
                    DC_CLIENT | ENABLE_CACHE | ENABLE_WRITE_BUFFER);
  }

  prev_heap_end = heap_end;
  heap_end += incr;
  uint32_t n_sections = (heap_end - HEAP_BASE + 1024*1024 - 1) >> 20;
  if (n_sections > allocated_pages) {
    // TODO: Proper error handling
    page_list_t* res = paging_allocate(n_sections - allocated_pages);
    while (res != NULL) {
      for (int i=0;i<res->size;i++) {
        mmu_add_section(
          KERNEL_TTB_ADDRESS,
          HEAP_BASE + allocated_pages*PAGE_SECTION,
          (res->address + i)*PAGE_SECTION,
          DC_CLIENT | ENABLE_CACHE | ENABLE_WRITE_BUFFER);
        allocated_pages++;
      }
      res = res->next;
    }
  } else if (n_sections < allocated_pages) {
      // TODO: Deallocate
  }

  return (uintptr_t)prev_heap_end;
}

int min(int const a, int const b) {
    return a < b ? a : b;
}

int max(int const a, int const b) {
    return a > b ? a : b;
}
