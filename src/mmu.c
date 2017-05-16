#include "mmu.h"
#include "stdlib.h"
#include "debug.h"
#include "arm.h"
#include "kernel.h"

extern int __kernel_phy_start;
extern int __kernel_phy_end;


/** \fn void mmu_setup_ttbcr(uint32_t N) {
 *  \brief Setup the ttbcr value
 *  \param N use ttb 1 iff address is superior to 2^(32-N)
 */
void mmu_setup_ttbcr(uint32_t N) {
    kdebug(D_MEMORY, 1, "TTBCR => (%d)\n", N);
    asm("mcr p15, 0, %0, c2, c0, 2\n"
        :
        : "r" (N)
        : );
}

/** \fn void mmu_set_ttb_1(uint32_t addr) {
 *  \brief Set the table for ttb1
 *  \param addr The address of the table
 *  \warning The table needs to be aligned to 16kb
 */
void mmu_set_ttb_1(uint32_t addr) {
    uint32_t reg;
    if (addr & ((1 << 14) - 1)) {
        kdebug(D_MEMORY, 8, "TTB1 address is not correctly aligned (%#010x)\n", addr);
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

    kdebug(D_MEMORY, 1, "TTB1 address updated (%#010x)\n", addr);
}

/** \fn void mmu_set_ttb_1(uint32_t addr) {
 *  \brief Set the table for ttb1
 *  \param addr The address of the table
 *  \param N The value of ttbcr
 *  \warning The table needs to be aligned to 2^(14-N) bits
 */
void mmu_set_ttb_0(uint32_t addr, uint32_t N) {
    uint32_t reg;
    if (addr & ((1 << (14-N)) - 1)) {
        kdebug(D_MEMORY, 8, "TTB0 address is not correctly aligned (%#010x %d)\n", addr, N);
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
	flush_prefetch_buffer();
	flush_instruction_cache();
	dmb();
	dsb();
	isb();

    kdebug(D_MEMORY, 1, "TTB0 address updated (%#010x %d)\n", addr, N);
}



/** \fn uintptr_t mmu_vir2phy_ttb(uintptr_t addr, uintptr_t ttb_phy)
 *  \brief Return the physical address of a virtual address
 *  \param addr The virtual address
 *  \param ttb_phy The ttb address
 *  \return The physical address of the one given in parameter
 */
uintptr_t mmu_vir2phy_ttb(uintptr_t addr, uintptr_t ttb_phy) {
	uintptr_t* address_section = (uintptr_t*)(0x80000000 | ttb_phy | ((addr & 0xFFF00000) >> 18));
    uintptr_t target_section = *address_section;
	if ((target_section & 3) != 0) {
	    target_section &= 0xFFF00000;
	    return target_section | (addr & 0x000FFFFF);
	} else {
		return -1;
	}
}

/** \fn uintptr_t mmu_vir2phy(uintptr_t addr)
 *  \brief Return the physical address of a virtual address (using TTB1)
 *  \param addr The virtual address
 *  \return The physical address of the one given in parameter
 */
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


/** \fn void mmu_stop()
 *  \brief Stop the mmu
 */
void mmu_stop() {
  asm("mrc p15,0,r2,c1,c0,0\n"
      "bic r2,#0x1000\n"
      "bic r2,#0x0004\n"
      "bic r2,#0x0001\n"
      "mcr p15,0,r2,c1,c0,0\n");
}

/** \fn void mmu_start()
 *  \brief Start the mmu
 */
void mmu_start() {
  asm("mrc p15,0,r2,c1,c0,0\n"
      "orr r2,r2,#1\n"
      "mcr p15,0,r2,c1,c0,0\n");
}


/** \fn void mmu_setup_ttb(uintptr_t ttb_address)
 *  \brief Setup a basic ttb on a given address (with the kernel in the higher half)
 *  \param ttb_address The address of the ttb to be construct
 *  \warning ttb_address should be 16kb aligned
 */
void mmu_setup_ttb(uintptr_t ttb_address) {
    uintptr_t i;
    for(i=0; i<0x80000000; i+=0x100000) {
        mmu_delete_section(ttb_address,i);
    }

    for(; i<0xbf000000; i+=0x100000) {
        mmu_add_section(ttb_address,i,i-0x8000000,ENABLE_CACHE | ENABLE_WRITE_BUFFER, 0, AP_PRW_UNONE);
    }

    for(; i<0xc0000000; i+=0x100000) {
        #ifdef RPI2
        mmu_add_section(ttb_address,i,i-0x8000000, 0, 0, AP_PRW_UNONE);
        #else
        mmu_add_section(ttb_address,i,i-0x9f00000, 0, 0, AP_PRW_UNONE);
        #endif
    }

    for(; i<0xf0000000; i+=0x100000) {
        mmu_delete_section(ttb_address,i);
    }

    int kernel_image_size = &__kernel_phy_end - &__kernel_phy_start;

    for(int j=0,k=0; j<0x100 && k<kernel_image_size; j++,i+=0x100000,k+=0x100000) {
        mmu_add_section(ttb_address,i,k,ENABLE_CACHE | ENABLE_WRITE_BUFFER, 0, AP_PRW_UNONE);
    }
}


/** \fn void mmu_setup_coarse_table(uintptr_t coarse_table_address, uintptr_t ttb_address, uintptr_t from)
 *  \brief Setup a coarse table on a given address
 *  \param coarse_table_address The address where to construct the coarse table
 *  \param ttb_address The address of the ttb which will use the coarse table
 *  \param from The physical address wich will redirect to the coarse table
 *  \warning coarse_table_address should be 2^10 bits aligned
 */
void mmu_setup_coarse_table(uintptr_t coarse_table_address, uintptr_t ttb_address, uintptr_t from) {
    //We link the second level ttb to the ttb
    uintptr_t address_section = (ttb_address | (uintptr_t)((from & 0xFFF00000) >> 18));
    uint32_t value_section = (0xFFFFFC00 & coarse_table_address) | COARSE_PAGE_TABLE;
    *((uint32_t*)(address_section)) = value_section;

    //We setup all the pages
    for(uint32_t i=0; i<NB_PAGES_COARSE_TABLE; i++) {
        *(uint32_t*)((coarse_table_address & 0xFFFFFC00) | (i<<2)) = 0;
    }
}

/** \fn void mmu_setup_fine_table(uintptr_t fine_table_address, uintptr_t ttb_address, uintptr_t from)
 *  \brief Setup a fine table on a given address
 *  \param fine_table_address The address where to construct the fine table
 *  \param ttb_address The address of the ttb which will use the fine table
 *  \param from The physical address wich will redirect to the fine table
 *  \warning fine_table_address should be 2^12 bits aligned
 */
void mmu_setup_fine_table(uintptr_t fine_table_address, uintptr_t ttb_address, uintptr_t from) {
    //We link the second level ttb to the ttb
    uintptr_t address_section = (ttb_address | (uintptr_t)((from & 0xFFF00000) >> 18));
    uint32_t value_section = (0xFFFFF000 & fine_table_address) | FINE_PAGE_TABLE;
    *((uint32_t*)(address_section)) = value_section;

    //We setup all the pages
    for(uint32_t i=0; i<NB_PAGES_FINE_TABLE; i++) {
        *(uint32_t*)((fine_table_address & 0xFFFFF000) | (i<<2)) = 0;
    }
}



/** \fn void mmu_add_section(uintptr_t ttb_address, uintptr_t from, uintptr_t to, uint32_t flags, uint32_t domain, uint32_t ap)
 *  \brief Add a redirection of a physical address to a ttb
 *  \param ttb_address The address of the ttb
 *  \param from The physical address base to redirect from
 *  \param to The virtual address base to redirect to
 *  \param flags The flags used for the ttb
 *  \param domain The domain of the page
 *  \param ap The access permissions bits
 *  \warning from and to should be 2^20 bits aligned
 */
void mmu_add_section(uintptr_t ttb_address, uintptr_t from, uintptr_t to,
                     uint32_t flags, uint32_t domain, uint32_t ap) {
	if (flags >= 4 || domain >= 16 || ap >= 4) {
		while(1) {} // Trap
	}
    uintptr_t address_section = (ttb_address | (uintptr_t)((from & 0xFFF00000) >> 18));
    uint32_t value_section = (0xFFF00000 & to) | (flags << 2) | (domain << 5) | (ap << 10) | SECTION;
    *((uint32_t*)(address_section)) = value_section;
}


/** \fn void mmu_delete_section(uintptr_t ttb_address, uintptr_t address)
 *  \brief Delete a section
 *  \param ttb_address The address of the ttb
 *  \param address The base address to delete
 */
void mmu_delete_section(uintptr_t ttb_address, uintptr_t address) {
    *(uint32_t*)(ttb_address | ((address & 0xFFF00000) >> 18)) = 0;
}


/** \fn void mmu_add_small_page(uintptr_t coarse_table_address, uintptr_t from, uintptr_t to, uint32_t flags) {
 *  \brief Add a small page to a coarse table
 *  \param coarse_table_address The address of the coarse table
 *  \param from The base address of the physical page
 *  \param to The base address of the virtual page
 *  \param flags The flags used in the coarse table
 */
void mmu_add_small_page(uintptr_t coarse_table_address, uintptr_t from, uintptr_t to, uint32_t flags) {
    uintptr_t address = (coarse_table_address & 0xFFFFFC00) | ((from & 0xFF000) >> 10);
    uint32_t value = (to & 0xFFFFF000) | (0xFF0) | flags | SMALL_PAGE;
    *((uint32_t*)(address)) = value;
}


/** \fn void mmu_delete_small_page(uintptr_t coarse_table_address, uintptr_t address)
 *  \brief Delete a small page in a coarse table
 *  \param coarse_table_address The base address of the table
 *  \param address The base address of the small page
 */
void mmu_delete_small_page(uintptr_t coarse_table_address, uintptr_t address) {
    *(uint32_t*)(coarse_table_address | ((address & 0xFF000) >> 10)) = 0;
}


/** \fn void mmu_add_tiny_page(uintptr_t fine_table_address, uintptr_t from, uintptr_t to, uint32_t flags) {
 *  \brief Add a tiny page to a fine table
 *  \param fine_table_address The address of the fine table
 *  \param from The base address of the physical page
 *  \param to The base address of the virtual page
 *  \param flags The flags used in the fine table
 */
void mmu_add_tiny_page(uintptr_t fine_table_address, uintptr_t from,
                        uintptr_t to, uint32_t flags) {
    uintptr_t address = (fine_table_address & 0xFFFFF000) | ((from & 0xFFC00) >> 8);
    uint32_t value = (to & 0xFFFFFC00) | (0x30) | flags | TINY_PAGE;
    *((uint32_t*)(address)) = value;
}


/** \fn void mmu_delete_tiny_page(uintptr_t fine_table_address, uintptr_t address)
 *  \brief Delete a tiny page in a fine table
 *  \param coarse_table_address The base address of the table
 *  \param address The base address of the tiny page
 */
void mmu_delete_tiny_page(uintptr_t fine_table_address, uintptr_t address) {
    *(uint32_t*)(fine_table_address | ((address & 0xFFC00) >> 8)) = 0;
}
