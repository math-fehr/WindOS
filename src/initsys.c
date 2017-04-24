#include "stdint.h"
#include "stddef.h"
#include "mmu.h"

#define KERNEL_PHY_TTB_ADDRESS 0x4000

extern int __kernel_phy_end;



void init_map(uintptr_t from, uintptr_t to, uint32_t flags) {
  uintptr_t address_section = (KERNEL_PHY_TTB_ADDRESS | (uintptr_t)((from & 0xFFF00000) >> 18));
  uint32_t value_section = (0xFFF00000 & to) | flags | SECTION;
  *((uint32_t*)(address_section)) = value_section;
}

void init_setup_ttb() {
  uintptr_t i;
  const uint32_t section_size = 0x00100000;
  for(i=0;i<0x80000000;i+=section_size) { // temporary linear mapping (deleted as soon as we enter real kernel mode)
    init_map(i,i,DC_CLIENT);
  }

  for(;i<0xbf000000;i+=section_size) { // physical ram mapping for kernel.
      init_map(i,i-0x80000000,ENABLE_CACHE | ENABLE_WRITE_BUFFER | DC_CLIENT);
  }

  for(;i<0xc0000000;i+=section_size) { // peripherals mapping
    #ifdef RPI2
    init_map(i,i-0x80000000,DC_CLIENT);
    #else
    init_map(i,i-0x9f000000,DC_CLIENT);
    #endif
  }
  for(i=0xf0000000;i<0xf0000000+(uintptr_t)&__kernel_phy_end;i+=section_size) { // kernel data & code mapping
    init_map(i,i-0xf0000000,ENABLE_CACHE | ENABLE_WRITE_BUFFER | DC_CLIENT);
  }
}

uint32_t init_get_ram() {
    uint32_t* atags_ptr = (uint32_t*)0x100;

    #define ATAG_NONE 0
    #define ATAG_CORE 0x54410001
    #define ATAG_MEM  0x54410002

	while (*(atags_ptr+1) != ATAG_NONE) {
		if (*(atags_ptr+1) == ATAG_MEM) {
            return *(atags_ptr+2);
		}
		atags_ptr += (*atags_ptr);
	}
    return (uint32_t)512*(uint32_t)1026*(uint32_t)1024*(uint32_t)1024;
}
