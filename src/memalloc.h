#ifndef PAGING_H
#define PAGING_H


#include "stddef.h"
#include "stdint.h"

#include "debug.h"

//http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0198e/Babegida.html
#define PAGE_SECTION 1024*1024
#define PAGE_LARGE   64*1024
#define PAGE_SMALL   4*1024
#define PAGE_TINY    1*1024

typedef struct page_t page_t;
struct page_t{
  int size;
  int address;
  page_t* next;
};

typedef struct page_list_t page_list_t;
struct page_list_t {
  page_list_t* next;
  int size; // in number of tiny pages.
  int address; // tiny page-aligned (1->1024).
};

#endif
