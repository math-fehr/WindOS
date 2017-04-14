#ifndef PAGING_H
#define PAGING_H

#include "stdint.h"

//Number of possible pages
#define NB_ENTRIES_TTB 4096

/**
 * Definition of 2 bits domain control for paging
 */
#define DC_2B_NO_ACCESS (0b00 << 10)  //Accesses generates a domain fault
#define DC_2B_CLIENT    (0b01 << 10)  //Accesses are checked with access permission bits
#define DC_2B_RESERVED  (0b10 << 10)  //Using this value produce unpredictable values
#define DC_2B_MANAGER   (0b11 << 10)  //Permission fault are not generated

/**
 * Definition of other flags
 */
#define ENABLE_CACHE        (1 << 3) //Use the cache
#define ENABLE_WRITE_BUFFER (1 << 2) //Enable write buffer
#define SECTION_1MB         (1 << 1) //The flag to have a 1Mb section


#define TTB_ADRESS 0x4000  //The adress of the table translation base

void mmu_setup_mmu();

void mmu_add_section(uint32_t const from, uint32_t const to, uint32_t const flags);

#endif //PAGING_H
