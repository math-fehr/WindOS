#include "stdint.h"
#include "stddef.h"
#include "mmu.h"
#include "gpio.h"
#include "arm.h"

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
  for(i=0;i<0x80000000;i+=section_size) { // temporary linear mapping (deleted as soon as we enter real kernel code)
    init_map(i,i,0);
  }

  for(;i<0xbf000000;i+=section_size) { // physical ram mapping for kernel.
    init_map(i,i-0x80000000,0);
  }

  for(;i<0xc0000000;i+=section_size) { // peripherals mapping
    #ifdef RPI2
    init_map(i,i-0x80000000,0);
    #else
    init_map(i,i-0x9f000000,0);
    #endif
  }
  for(i=0xf0000000;i<0xf0000000+(uintptr_t)&__kernel_phy_end;i+=section_size) { // kernel data & code mapping
    init_map(i,i-0xf0000000,0);
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

void sys_init() {
    uint32_t r = mrc(p15, 0, c1, c0, 1); // Read aux registers
    r |= (1<<6); // smp bit
    mcr(p15, 0, c1, c0, 1, r);
    isb();
    dsb();

    flush_data_cache(true);
    flush_instruction_cache();
    tlb_flush_all();
    dsb();
    isb();

    init_setup_ttb();

    mcr(p15,0,c2,c0,0,0x4000 | (1 << 3) | (1 << 6)); // TTB0
    mcr(p15,0,c2,c0,1,0x4000 | (1 << 3) | (1 << 6)); // TTB1
    mcr(p15,0,c2,c0,2,0); // TTBCR
    dsb();
    isb();

    mcr(p15,0,c3,c0,0, ~0); // domain

    uint32_t mmu_ctrl = mrc(p15,0,c1,c0,0);
    mmu_ctrl |= (1 << 11) | (1 << 2) | (1 << 12) | (1 << 0) | (1 << 5);

    mcr(p15,0,c1,c0,0,mmu_ctrl);
    isb();

	mcr(p15,0,c12,c0,0,0xf0000000); // set interrupt vector base address

    tlb_flush_all();
    dsb();
    isb();
}
