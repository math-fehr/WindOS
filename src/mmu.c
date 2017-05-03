#include "mmu.h"
#include "stdlib.h"
#include "debug.h"

extern int __kernel_phy_start;
extern int __kernel_phy_end;


// Setups ttb control register in order to have two ttb
// (one for the OS and one for the program)
// N = 0 => use only ttb 0
// N > 0 => use ttb 1 if (vaddr & (1110.....0))
//                                 |||-> N
void mmu_setup_ttbcr(uint32_t N) {
    kdebug(D_MEMORY, 1, "TTBCR => (%d)\n", N);
    asm("mcr p15, 0, %0, c2, c0, 2\n"
        :
        : "r" (N)
        : );
}

void mmu_set_ttb_1(uint32_t addr) {
    uint32_t reg;
    if (addr & ((1 << 14) - 1)) {
        kdebug(D_MEMORY, 10, "TTB1 address is not correctly aligned (%#010x)\n", addr);
        // TTB address is not correctly aligned. (16kb)
        return;
    }

    asm("mrc p15, 0, %0, c2, c0, 1\n"
        : "=r" (reg)
        :
        :);
    reg = reg & ((1 << 14) - 1); // mask previous address
    reg = reg | addr;
    asm("mcr p15, 0, %0, c2, c0, 1\n"
        :
        : "r" (reg)
        :);

    mmu_invalidate_unified_tlb();
    mmu_invalidate_caches();

    kdebug(D_MEMORY, 1, "TTB1 address updated (%#010x)\n", addr);
}

void mmu_set_ttb_0(uint32_t addr, uint32_t N) {
    uint32_t reg;
    if (addr & ((1 << (14-N)) - 1)) {
        kdebug(D_MEMORY, 10, "TTB0 address is not correctly aligned (%#010x %d)\n", addr, N);
        // TTB address is not correctly aligned. (16kb/2^N)
        return;
    }

    asm("mrc p15, 0, %0, c2, c0, 0\n"
        : "=r" (reg)
        :
        :);
    reg = reg & ((1 << (14-N)) - 1); // mask previous address
    reg = reg | addr;
    asm("mcr p15, 0, %0, c2, c0, 0\n"
        :
        : "r" (reg)
        :);

    mmu_invalidate_unified_tlb();
    mmu_invalidate_caches();

    kdebug(D_MEMORY, 1, "TTB0 address updated (%#010x %d)\n", addr, N);
}

uintptr_t mmu_vir2phy_ttb(uintptr_t addr, uintptr_t ttb_phy) {
	uintptr_t* address_section = (uintptr_t*)(0x80000000 | ttb_phy | ((addr & 0xFFF00000) >> 18));
    uintptr_t target_section = *address_section;
    target_section &= 0xFFF00000;
    return target_section | (addr & 0x000FFFFF);
}

uintptr_t mmu_vir2phy(uintptr_t addr) {
    uintptr_t ttb_phy;
    if (addr & (((1 << TTBCR_ALIGN)-1) << (32 - TTBCR_ALIGN))) { // USE TTB1
        asm("mrc p15, 0, %0, c2, c0, 1\n"
            : "=r" (ttb_phy)
            :
            :);
        ttb_phy &= ~((1 << 13) - 1);
    } else {
        asm("mrc p15, 0, %0, c2, c0, 0\n"
            : "=r" (ttb_phy)
            :
            :);
        ttb_phy &= ~((1 << (13 - TTBCR_ALIGN)) - 1);
    }

	uintptr_t* address_section = (uintptr_t*)(0x80000000 | ttb_phy | ((addr & 0xFFF00000) >> 18));
    uintptr_t target_section = *address_section;
    target_section &= 0xFFF00000;
    return target_section | (addr & 0x000FFFFF);
}

inline void dsb() {
  asm("mcr p15, 0, %0, c7,c10,4"
      :
      : "r" (0)
      :);
}


void mmu_stop() {
  asm("mrc p15,0,r2,c1,c0,0\n"
      "bic r2,#0x1000\n"
      "bic r2,#0x0004\n"
      "bic r2,#0x0001\n"
      "mcr p15,0,r2,c1,c0,0\n");
}

void mmu_start() {
  asm("mrc p15,0,r2,c1,c0,0\n"
      "orr r2,r2,#1\n"
      "mcr p15,0,r2,c1,c0,0\n");
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
