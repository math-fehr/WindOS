#include "memalloc.h"
#include "stdlib.h"

 
page_t* free_list;


// sort by decreasing order of size.
page_t* insertion(page_t* elem, page_t* list) {
  if (elem->size >= list->size) {
    elem->next = list;
    return elem;
  } else {
    page_t* prev = NULL;
    page_t* first = list;
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
  free_list = malloc(sizeof(page_t));
  free_list->next = NULL;
  free_list->size = n_total_pages-n_reserved_pages;
  free_list->address = n_reserved_pages;
}

page_list_t* paging_allocate(int n_pages) {
  page_list_t* result = NULL;
  page_t* pointer = free_list;
  page_t* previous = NULL;

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

    page_t *tmp = pointer->next;
    previous->next = pointer->next;
    if (pointer->size == 0) {
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
  }

  return result;
}


void paging_free(int n_pages, int address) {
  page_t* elem = malloc(sizeof(page_t));
  elem->size = n_pages;
  elem->address = address;
  elem->next = NULL;

  free_list = insertion(elem, free_list);
}
