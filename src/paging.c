#include "paging.h"
#include "stdlib.h"



void mmu_setup_ttb_kernel(uintptr_t ttb_address) {
    for(uint32_t i=0, j=0; i<NB_SECTION_TTB; i++,j+=0x100000) {
        mmu_add_section(ttb_address,j,j,ENABLE_CACHE | ENABLE_WRITE_BUFFER);
    }

    //We desactivate the cache for the peripherals
    #ifdef RPI2
    mmu_add_section(ttb_address,0x3F000000,0x3F000000,0x00000);
    mmu_add_section(ttb_address,0x3F000000,0x3F200000,0x00000);
    #else
    mmu_add_section(ttb_address,0x20000000,0x20000000,0x00000);
    mmu_add_section(ttb_address,0x20200000,0x20200000,0x00000);
    #endif
}


void mmu_setup_ttb(uintptr_t ttb_address) {
    for(uint32_t i=0; i<NB_SECTION_TTB; i++) {
        //No memory is mapped
        //Trying to access to RAM will generate a data abort
        *(uint32_t*)(ttb_address + (i << 2)) = 0;
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
    uintptr_t address = (fine_table_address & 0xFFFFFC00) | ((from & 0xFF000) >> 10);
    uint32_t value = (to & 0xFFFFF000) | (0x30) | flags | TINY_PAGE;
    *((uint32_t*)(address)) = value;
}

void mmu_delete_tiny_page(uintptr_t fine_table_address, uintptr_t address) {
    *(uint32_t*)(fine_table_address | ((address & 0xFFC00) >> 8)) = 0;
}
