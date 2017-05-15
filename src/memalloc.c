/** \file memalloc.c
 * 	\brief Memory allocation features
 *
 *	Here we cut memory into 'pages', the pages here being sections (1MiB).
 * 	These are memory allocation functions.
 */

#include "memalloc.h"
#include "stdlib.h"
#include "malloc.h"


/** \var page_list_t* free_list
 *	\brief List of available pages.
 */
page_list_t* free_list;

/** \var int tot_pages
 * 	\brief Total amount of pages counter.
 */
int tot_pages;

/** \var int used_pages
 * 	\brief Total amount of allocated pages counter.
 */
int used_pages;


/** \fn page_list_t* insertion(page_list_t* elem, page_list_t* list)
 *	\brief Inserts an element in a list, sorting by ascending address.
 * 	\param elem The element to insert.
 *	\param list The destination list.
 * 	\return The new list.
 */
page_list_t* insertion(page_list_t* elem, page_list_t* list) {
	if (list == NULL) {
		return elem;
	} else if(elem == NULL) {
		return list;
	}

  	if (elem->address <= list->address) {
    	elem->next = list;
    	return elem;
  	} else {
	    page_list_t* prev = NULL;
	    page_list_t* first = list;
	    while (list != NULL && elem->address > list->address) {
	      prev = list;
	      list = list->next;
	    }
	    prev->next = elem;
	    elem->next = list;
	    return first;
 	}
}

/**	\fn void paging_init(int n_total_pages, int n_reserved_pages)
 *	\brief Initialize paging structure.
 *	\param n_total_pages The amount of pages in the memory.
 *	\param n_reserved_pages The amount of pages reserved for kernel code/data.
 */
void paging_init(int n_total_pages, int n_reserved_pages) {
	tot_pages = n_total_pages;
	used_pages = n_reserved_pages;
	free_list = malloc(sizeof(page_list_t));
	free_list->next = NULL;
	free_list->size = n_total_pages-n_reserved_pages;
	free_list->address = n_reserved_pages;
}

/** \fn paging_print_start()
 * 	\brief Draw a character representation of memory.
 */
void paging_print_status() {
	char* bitmap = malloc(tot_pages+1);
	for(int i=0;i<tot_pages;i++) {
		bitmap[i] = '#';
	}

	page_list_t* fl = free_list;
	while(fl != NULL) {
		for (int i=fl->address;i<fl->address+fl->size;i++) {
			bitmap[i] = '.';
		}
		fl = fl->next;
	}
	bitmap[tot_pages] = 0;
	kdebug(D_KERNEL,3,"%s\n", bitmap);
}

/**	\fn page_list_t* paging_allocate(int n_pages)
 *	\brief Allocates the request number of page.
 *	\param n_pages The number of pages to allocate.
 *	\return On success, a list of allocated pages. On fail, NULL pointer.
 *
 *	A greedy algorithm is used to find pages.
 *
 * 	\bug If the kernel sbrk call this function, a loop creates as this function
 *	uses malloc.
 */
page_list_t* paging_allocate(int n_pages) {
	page_list_t* result = NULL;
	page_list_t* pointer = free_list;

	kdebug(D_KERNEL,1,"Alloc %d\n",n_pages);
	used_pages += n_pages;


	if (100*used_pages / tot_pages > 90 && 100*(used_pages-n_pages) / tot_pages <= 90) {
		kdebug(D_KERNEL,10,"90%% of memory is used.\n");
	}
	page_list_t *tmp = NULL;

	while (n_pages > 0 && pointer != NULL) {
	    int mn = min(pointer->size, n_pages);
	    n_pages -= mn;
	    pointer->size -= mn;

	    page_list_t *elem = malloc(sizeof(page_list_t));
		if (elem == NULL) {
			serial_write("MALLOC FAILED I'M DONE.\n");
			while(1) {} // dafuk.
		}
		elem->next = result;
	    elem->size = mn;
	    elem->address = pointer->address;
		//kernel_printf("elemn of size %d address %d\n", mn, elem->address);
	    result = elem;

		pointer->address += mn;
	    tmp = pointer->next;

	    if (pointer->size == 0) {
			free(pointer);
			pointer = tmp;
		}
  	}
	free_list = tmp;

	if (pointer != NULL && pointer != free_list) {
		pointer->next = NULL;
		free_list = insertion(pointer, tmp);
	}

	if (n_pages > 0) {
    	kdebug(D_KERNEL, 6, "Out of memory error.\n");
	} else {
		kdebug(D_KERNEL, 1, "%d/%d pages in use.\n", used_pages, tot_pages);
	}
	paging_print_status();
	return result;
}

/**	\fn void paging_free(int n_pages, int address)
 * 	\brief Free pages.
 * 	\param n_pages The number of consecutive pages to free.
 *	\param address The address of the first page to free.
 *
 *	\warning No checks are done during the free.
 */
void paging_free(int n_pages, int address) {
	kdebug(D_KERNEL, 1, "free %d %d.\n", n_pages, address);
	page_list_t* elem = malloc(sizeof(page_list_t));
	elem->size = n_pages;
	elem->address = address;
	elem->next = NULL;

	used_pages -= n_pages;

	free_list = insertion(elem, free_list);
	kdebug(D_KERNEL, 1, "%d/%d pages in use.\n", used_pages, tot_pages);
	paging_print_status();
}


/** \fn int memalloc(uint32_t ttb_address, uintptr_t address, size_t size)
 *	\brief Allocates memory and update the translation table.
 *	\param ttb_address Translation table address.
 *	\param address Virtual address where we want to allocate pages.
 *	\param size The amount of requested memory allocation.
 *	\return 0
 *
 * 	\warning Works when the paging is set up for 4kb pages.
 */
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
                            ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW);//TODO change flags
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
