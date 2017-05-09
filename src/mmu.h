#ifndef MMU_H
#define MMU_H

#include "stdint.h"

//Number of possible pages
#define NB_SECTION_TTB 4096
#define NB_PAGES_COARSE_TABLE 256
#define NB_PAGES_FINE_TABLE 1024

//Size of pages
#define PAGE_SECTION (1024*1024)
#define PAGE_LARGE   (64*1024)
#define PAGE_SMALL   (4*1024)
#define PAGE_TINY    (1*1024)

/**
 * Definition of 2 bits domain control for paging
 */
#define DC_NO_ACCESS 0b00  //Accesses generates a domain fault
#define DC_CLIENT    0b01  //Read/Write for Priviledged, and no access for User
#define DC_RESERVED  0b10  //Read/Write for Priviledged, and Read for User
#define DC_MANAGER   0b11  //Read/Write for User and Priviledged

#define TTBCR_ALIGN_16K 0
#define TTBCR_ALIGN_8K  1
#define TTBCR_ALIGN_4K  2
#define TTBCR_ALIGN_2K  3
#define TTBCR_ALIGN_1K  4
#define TTBCR_ALIGN_512 5
#define TTBCR_ALIGN_256 6
#define TTBCR_ALIGN_128 7
/**
 * The domain bits are the 8:5 bits
 */

/**
 * Definition of other flags
 */
#define ENABLE_CACHE        2 //Use the cache
#define ENABLE_WRITE_BUFFER 1 //Enable write buffer

/*
 * Access Permission bits
 */
#define AP_PNONE_UNONE		0
#define AP_PRW_UNONE 		1
#define AP_PRW_URO 			2
#define AP_PRW_URW 			3

/**
 * Definition of formats for first-level descriptor
 */
#define UNALLOWED_SECTION   0b00 //declare an unallowed section
#define SECTION             0b10 //define a 1Mb section
#define COARSE_PAGE_TABLE   0b01 //redirect a section to a coarse page table
#define FINE_PAGE_TABLE     0b11 //redirect a section to a fine page table


/**
 * Definition of formats for second-level descriptor
 */
#define UNALLOWED_PAGE      0b00 //defines an unallowed page
#define LARGE_PAGE          0b10 //defines a 64kb page
#define SMALL_PAGE          0b01 //defines a 4kb page
#define TINY_PAGE           0b11 //defines a 1kb page


void mmu_setup_ttbcr(uint32_t N);
void mmu_set_ttb_1(uint32_t addr);
void mmu_set_ttb_0(uint32_t addr, uint32_t N);

uintptr_t mmu_vir2phy_ttb(uintptr_t addr, uintptr_t ttb_phy);
uintptr_t mmu_vir2phy(uintptr_t reg); // performs a table walk to get the physical address.

static inline void mmu_invalidate_caches() {
  asm("mov r0, #0\n"
      "mcr p15,0,r0,c7,c7,0");
}
static inline void mmu_invalidate_unified_tlb() {
  asm("mov r0, #0\n"
      "mcr p15,0,r0,c8,c7,0\n");
}

void mmu_stop();
void mmu_start();

//ttb_address should be 16kb aligned
void mmu_setup_ttb(uintptr_t ttb_address);


void mmu_setup_coarse_table(uintptr_t coarse_table_address, uintptr_t ttb_address,
                            uintptr_t from);


void mmu_setup_fine_table(uintptr_t fine_table_address, uintptr_t ttb_address,
                          uintptr_t from);


void mmu_add_section(uintptr_t ttb_address, uintptr_t from,
                     uintptr_t to, uint32_t flags, uint32_t domain, uint32_t ap);


void mmu_delete_section(uintptr_t ttb_address, uintptr_t address);


void mmu_add_small_page(uintptr_t coarse_table_address, uintptr_t from,
                        uintptr_t to, uint32_t flags);


void mmu_delete_small_page(uintptr_t coarse_table_address, uintptr_t address);


void mmu_add_tiny_page(uintptr_t fine_table_address, uintptr_t from,
                       uintptr_t to, uint32_t flags);


void mmu_delete_tiny_page(uintptr_t fine_table_address, uintptr_t address);

void free_section(uintptr_t ttb_address, uintptr_t section);

#endif //PAGING_H
