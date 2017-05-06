#include "memalloc.h"
#include "stdlib.h"
#include "malloc.h"


page_list_t* free_list;

int tot_pages;
int used_pages;

// sort by decreasing order of size.
page_list_t* insertion(page_list_t* elem, page_list_t* list) {
  if (elem->size >= list->size) {
    elem->next = list;
    return elem;
  } else {
    page_list_t* prev = NULL;
    page_list_t* first = list;
    while (list != NULL && elem->size < list->size) {
      prev = list;
      list = list->next;
    }
    prev->next = elem;
    elem->next = list;
    return first;
  }
}

void paging_init(int n_total_pages, int n_reserved_pages) {
	tot_pages = n_total_pages;
	used_pages = n_reserved_pages;
	free_list = malloc(sizeof(page_list_t));
	free_list->next = NULL;
	free_list->size = n_total_pages-n_reserved_pages;
	free_list->address = n_reserved_pages;
}

page_list_t* paging_allocate(int n_pages) {
  page_list_t* result = NULL;
  page_list_t* pointer = free_list;
  page_list_t* previous = NULL;

  used_pages += n_pages;

  while (n_pages > 0 && pointer != NULL) {
    int mn = min(pointer->size, n_pages);
    n_pages -= mn;
    pointer->size -= mn;

    page_list_t *elem = malloc(sizeof(page_list_t));
    elem->next = result;
    elem->size = mn;
    elem->address = pointer->address;
    result = elem;

    pointer->address += mn;

    page_list_t *tmp = pointer->next;
    if (pointer->size == 0) {
      previous->next = pointer->next;
      free(pointer);
      pointer = NULL;
    } else {
      previous = pointer;
    }
    pointer = tmp;
  }

	if (pointer != NULL) {
		free_list = insertion(pointer, free_list);
	}

	if (n_pages > 0) {
    	kdebug(D_KERNEL, 6, "Out of memory error.\n");
	} else {
		kdebug(D_KERNEL, 1, "%d/%d pages in use.\n", used_pages, tot_pages);
	}

  return result;
}


void paging_free(int n_pages, int address) {
  page_list_t* elem = malloc(sizeof(page_list_t));
  elem->size = n_pages;
  elem->address = address;
  elem->next = NULL;

  used_pages -= n_pages;

  free_list = insertion(elem, free_list);
}

int memalloc(uint32_t ttb_address, uintptr_t address, size_t size) {
    //We need the data to be aligned
    size = size + (address & 0xFFF) + 0xFFF;
    address = address & 0xFFFFF000;

    uintptr_t section = address & 0xFFF00000;
    uintptr_t small_page = address & 0xFF000;
    int nb_pages = size >> 12;
    page_list_t* physical_pages = paging_allocate(nb_pages);
    uintptr_t coarse_table_address;

    while(nb_pages != 0) {
        uint32_t value_section = *(uint32_t*)(ttb_address | ((address & 0xFFF00000) >> 18));

        //If we can allocate a section, we do it
        if(((value_section & 0b11) == 0b00) &&
           (nb_pages >= NB_PAGES_COARSE_TABLE) &&
           (physical_pages->size >= NB_PAGES_COARSE_TABLE) &&
           (small_page == 0)){

            mmu_add_section(ttb_address,
                            address,
                            physical_pages->address * 0x1000,
                            DC_MANAGER);//TODO change flags
            physical_pages->size -= NB_PAGES_COARSE_TABLE;
            physical_pages->address += NB_PAGES_COARSE_TABLE;
            address += 0x100000;
            nb_pages -= NB_PAGES_COARSE_TABLE;
            if(physical_pages->size == 0) {
                page_list_t* old = physical_pages;
                physical_pages = physical_pages->next;
                free(old);
            }
        }

        if((value_section & 0b11) == 0b00) {
            uintptr_t new_coarse_table = (uintptr_t)memalign(0x400,0x400);
            mmu_setup_coarse_table(new_coarse_table, ttb_address, section);
        }

        if((value_section & 0b11) == 0b01) {
            coarse_table_address = 0xFFFFFC00;
            for(; small_page < 0x100000; small_page += 0x1000) {
                mmu_add_small_page(coarse_table_address,
                                   address,
                                   physical_pages->address*0x1000,
                                   DC_MANAGER); //TODO change flags
                physical_pages->size--;
                physical_pages->address++;
                nb_pages--;
                address += 0x1000;
                if(physical_pages->size == 0) {
                    page_list_t* old = physical_pages;
                    physical_pages = physical_pages->next;
                    free(old);
                }
            }
        }
        else { //Error here -> We free the memory
            kdebug(D_MEMORY, 11, "The kernel try to allow memory on an already allowed place\n");
            while(physical_pages != NULL) {
                page_list_t* old = physical_pages;
                physical_pages = physical_pages->next;
                free(old);
            }
            return -1;
        }

        small_page = 0;
        section+= 0x100000;
    }
    return 0;
}
