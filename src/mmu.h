#ifndef PAGING_H
#define PAGING_H

#include "stdint.h"

//Number of possible pages
#define NB_SECTION_TTB 4096
#define NB_PAGES_COARSE_TABLE 256
#define NB_PAGES_FINE_TABLE 1024

//Size of pages
#define PAGE_SECTION 1024*1024
#define PAGE_LARGE   64*1024
#define PAGE_SMALL   4*1024
#define PAGE_TINY    1*1024

/**
 * Definition of 2 bits domain control for paging
 */
#define DC_NO_ACCESS 0b00  //Accesses generates a domain fault
#define DC_CLIENT    0b01  //Read/Write for Priviledged, and no access for User
#define DC_RESERVED  0b10  //Read/Write for Priviledged, and Read for User
#define DC_MANAGER   0b11  //Read/Write for User and Priviledged

/**
 * The domain bits are the 8:5 bits
 */

/**
 * Definition of other flags
 */
#define ENABLE_CACHE        (1 << 3) //Use the cache
#define ENABLE_WRITE_BUFFER (1 << 2) //Enable write buffer


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

uintptr_t mmu_p2v(uintptr_t addr);

//ttb_address should be 16kb aligned
void mmu_setup_ttb(uintptr_t ttb_address);


void mmu_setup_coarse_table(uintptr_t coarse_table_address, uintptr_t ttb_address,
                            uintptr_t from);


void mmu_setup_fine_table(uintptr_t fine_table_address, uintptr_t ttb_address,
                          uintptr_t from);


void mmu_add_section(uintptr_t ttb_address, uintptr_t from,
                     uintptr_t to, uint32_t flags);


void mmu_delete_section(uintptr_t ttb_address, uintptr_t address);


void mmu_add_small_page(uintptr_t coarse_table_address, uintptr_t from,
                        uintptr_t to, uint32_t flags);


void mmu_delete_small_page(uintptr_t coarse_table_address, uintptr_t address);


void mmu_add_tiny_page(uintptr_t fine_table_address, uintptr_t from,
                       uintptr_t to, uint32_t flags);


void mmu_delete_tiny_page(uintptr_t fine_table_address, uintptr_t address);

void free_section(uintptr_t ttb_address, uintptr_t section);

#endif //PAGING_H
