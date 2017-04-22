#include "mmu.h"
#include "stdlib.h"


extern int __kernel_phy_start;
extern int __kernel_phy_end;


uintptr_t mmu_p2v(uintptr_t addr) {
  if (addr < 0x80000000) {
    return addr + 0x80000000;
  } else {
    while(1) {}
  }
}

void mmu_setup_ttb(uintptr_t ttb_address) {
    uintptr_t i;
    for(i=0; i<0x80000000; i+=0x100000) {
        mmu_delete_section(ttb_address,i);
    }

    for(; i<0xbf000000; i+=0x100000) {
        mmu_add_section(ttb_address,i,i-0x8000000,ENABLE_CACHE | ENABLE_WRITE_BUFFER | DC_CLIENT);
    }

    for(; i<0xc0000000; i+=0x100000) {
        #ifdef RPI2
        mmu_add_section(ttb_address,i,i-0x8000000,DC_CLIENT);
        #else
        mmu_add_section(ttb_address,i,i-0x9f00000,DC_CLIENT);
        #endif
    }

    for(; i<0xf0000000; i+=0x100000) {
        mmu_delete_section(ttb_address,i);
    }

    int kernel_image_size = &__kernel_phy_end - &__kernel_phy_start;

    for(int j=0,k=0; j<0x100 && k<kernel_image_size; j++,i+=0x100000,k+=0x100000) {
        mmu_add_section(ttb_address,i,k,ENABLE_CACHE | ENABLE_WRITE_BUFFER | DC_CLIENT);
    }
}


void mmu_setup_coarse_table(uintptr_t coarse_table_address, uintptr_t ttb_address,
                            uintptr_t from) {
    //We link the second level ttb to the ttb
    uintptr_t address_section = (ttb_address | (uintptr_t)((from & 0xFFF00000) >> 18));
    uint32_t value_section = (0xFFFFFC00 & coarse_table_address) | COARSE_PAGE_TABLE;
    *((uint32_t*)(address_section)) = value_section;

    //We setup all the pages
    for(uint32_t i=0; i<NB_PAGES_COARSE_TABLE; i++) {
        *(uint32_t*)((coarse_table_address & 0xFFFFFC00) | (i<<2)) = 0;
    }
}

void mmu_setup_fine_table(uintptr_t fine_table_address, uintptr_t ttb_address,
                            uintptr_t from) {
    //We link the second level ttb to the ttb
    uintptr_t address_section = (ttb_address | (uintptr_t)((from & 0xFFF00000) >> 18));
    uint32_t value_section = (0xFFFFF000 & fine_table_address) | FINE_PAGE_TABLE;
    *((uint32_t*)(address_section)) = value_section;

    //We setup all the pages
    for(uint32_t i=0; i<NB_PAGES_FINE_TABLE; i++) {
        *(uint32_t*)((fine_table_address & 0xFFFFF000) | (i<<2)) = 0;
    }
}



void mmu_add_section(uintptr_t ttb_address, uintptr_t from,
                     uintptr_t to, uint32_t flags) {
    uintptr_t address_section = (ttb_address | (uintptr_t)((from & 0xFFF00000) >> 18));
    uint32_t value_section = (0xFFF00000 & to) | flags | SECTION;
    *((uint32_t*)(address_section)) = value_section;
}


void mmu_delete_section(uintptr_t ttb_address, uintptr_t address) {
    *(uint32_t*)(ttb_address | ((address & 0xFFF00000) >> 18)) = 0;
}


void mmu_add_small_page(uintptr_t coarse_table_address, uintptr_t from,
                  uintptr_t to, uint32_t flags) {
    uintptr_t address = (coarse_table_address & 0xFFFFFC00) | ((from & 0xFF000) >> 10);
    uint32_t value = (to & 0xFFFFF000) | (0xFF0) | flags | SMALL_PAGE;
    *((uint32_t*)(address)) = value;
}

void mmu_delete_small_page(uintptr_t coarse_table_address, uintptr_t address) {
    *(uint32_t*)(coarse_table_address | ((address & 0xFF000) >> 10)) = 0;
}

void mmu_add_tiny_page(uintptr_t fine_table_address, uintptr_t from,
                        uintptr_t to, uint32_t flags) {
    uintptr_t address = (fine_table_address & 0xFFFFF000) | ((from & 0xFFC00) >> 8);
    uint32_t value = (to & 0xFFFFFC00) | (0x30) | flags | TINY_PAGE;
    *((uint32_t*)(address)) = value;
}

void mmu_delete_tiny_page(uintptr_t fine_table_address, uintptr_t address) {
    *(uint32_t*)(fine_table_address | ((address & 0xFFC00) >> 8)) = 0;
}
