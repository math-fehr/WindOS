#ifndef MEMALLOC_H
#define MEMALLOC_H


#include "stddef.h"
#include "stdint.h"

#include "debug.h"
#include "mmu.h"

typedef struct page_list_t page_list_t;
struct page_list_t {
  page_list_t* next;
  int size; // in number of pages.
  int address; // page-aligned
};

void paging_init(int n_total_pages, int n_reserved_pages);
page_list_t* paging_allocate(int n_pages);
int memalloc(uint32_t ttb_address, uintptr_t address, size_t size);

#endif
