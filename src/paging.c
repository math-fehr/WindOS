#include "paging.h"

void mmu_setup_mmu() {
    #ifdef RPI2
    mmu_add_section(0x3F000000,0x3F000000,0x00000);
    mmu_add_section(0x3F000000,0x3F200000,0x00000);
    #else
    mmu_add_section(0x20000000,0x20000000,0x00000);
    mmu_add_section(0x20200000,0x20200000,0x00000);
    #endif

    for(uint32_t i=0, j=0; i<NB_ENTRIES_TTB; i++,j+=0x100000) {
        mmu_add_section(j,j,0x00000);
    }
}


void mmu_add_section(uint32_t const from, uint32_t const to, uint32_t const flags) {
    uint32_t mask = 0xFFF00000;
    uint32_t* adress_page = (uint32_t*)(TTB_ADRESS | ((from & mask) >> 18));
    uint32_t value_page = (mask & to) | flags | SECTION_1MB;
    *(adress_page) = value_page;
}
