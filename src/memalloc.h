#ifndef MEMALLOC_H
#define MEMALLOC_H


#include "stddef.h"
#include "stdint.h"

#include "debug.h"
#include "mmu.h"

typedef struct page_list_t page_list_t;

/** \struct page_list_t
 * 	\brief A structure representing a set of pages.
 */
struct page_list_t {
  page_list_t* next; ///< Next page set
  int size; ///< Size of page chunk, in number of pages.
  int address; ///< Offset of page chunk.
};

void paging_print_status();
void paging_free(int n_pages, int address);
void paging_init(int n_total_pages, int n_reserved_pages);
page_list_t* paging_allocate(int n_pages);
int memalloc(uint32_t ttb_address, uintptr_t address, size_t size);

#endif
