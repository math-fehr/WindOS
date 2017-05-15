/** \file cstubs.c
 * 	Some useful piece of code, and stubs for Newlib.
 */
#include "cstubs.h"


/**	\def HEAP_BASE
 * 	\brief Base virtual adresse of kernel heap.
 */
#define HEAP_BASE 0xc0000000
/**	\def KERNEL_TTB_ADDRESS
 * 	\brief Virtual adress of the kernel's translation table.
 */
#define KERNEL_TTB_ADDRESS 0xf0004000

/** \var extern int __kernel_phy_end
 * 	\brief The end of the kernel image as defined by the linker script.
 */
extern int __kernel_phy_end;
/** \var uintptr_t heap_end
 * 	\brief Kernel program break.
 */
uintptr_t heap_end = 0;

/**	\fn char* basename(char* path)
 *	\brief Computes the basename of a path.
 *
 *	\param path The path.
 *	\return The base name of the path.
 * 	\warning Data pointed by path is modified by the process.
 *
 * 	The base name of a path is the last element in it.
 * 	For exemple in /dev/null, it is null.
 * 	In /home/jack/, it is jack.
 */
char* basename(char* path)
{
	char* p;
	if (path == NULL || *path == '\0')
		return ".";
	p = path + strlen(path) - 1;
	while (*p == '/') {
		if (p == path)
			return path;
		*p-- = '\0';
	}
	while (p >= path && *p != '/')
		p--;
	return p + 1;
}


/**	\fn char* dirname(char* path)
 *	\brief Computes the dir name of a path.
 *
 *	\param path The path.
 *	\return The dir name of the path.
 * 	\warning Data pointed by path is modified by the process.
 *
 * 	The dir name is the directory in which lies the base name.
 * 	For exemple in /dev/null, it is /dev/
 * 	In /home/jack/, it is /home/
 *  In hello, it is .
 */
char* dirname(char* path)
{
	char* p;
	if (path == NULL || *path == '\0')
		return ".";

	p = path + strlen(path) - 1;
	while (*p == '/') {
		if (p == path)
			return path;
		*p-- = '\0';
	}
	while (p >= path && *p != '/' )
		p--;
	return
		p < path ? "." :
		p == path ? "/" :
		(*p = '\0', path);
}
/**	\fn ssize_t _write(int fd, const void* buf, ssize_t count)
 *	\brief Newlib stub, not implemented.
 */
ssize_t _write(int fd, const void* buf, ssize_t count) {
    (void)fd;
    (void)buf;
    (void)count;
	return 0;
}

/**	\fn ssize_t _read(int fd, const void* buf, ssize_t count)
 *	\brief Newlib stub, not implemented.
 */
ssize_t _read(int fd, const void* buf, ssize_t count) {
    (void)fd;
    (void)buf;
    (void)count;
	return 0;
}

/**	\fn off_t _lseek(int fd, off_t cnt, int mode)
 *	\brief Newlib stub, not implemented.
 */
off_t _lseek(int fd, off_t cnt, int mode) {
    (void)fd;
    (void)cnt;
    (void)mode;
    return 0;
}

/**	\fn int _read(int fd)
 *	\brief Newlib stub, not implemented.
 */
int _close(int fd) {
    (void)fd;
	return 0;
}

/**	\fn uintptr_t _sbrk(int incr)
 *	\brief Sbrk function used by Newlib's malloc.
 *	\param incr Increment value of which the program break will be changed.
 *  \return Pointer to the old program break.
 *	\warning Pages are not deallocated.
 *
 * 	This function simply updates the kernel program break and allocates pages if
 * 	needed.
 */
uintptr_t _sbrk(int incr) {
  static uint32_t allocated_pages = 0;
  uintptr_t prev_heap_end;
  if(heap_end == 0) { // first initialization of heap.
    heap_end = (uintptr_t) HEAP_BASE;
    allocated_pages = 2;

    mmu_add_section(KERNEL_TTB_ADDRESS,
                    HEAP_BASE,
                    (uintptr_t)&__kernel_phy_end + PAGE_SECTION - 1,
                    ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_UNONE);

    mmu_add_section(KERNEL_TTB_ADDRESS,
                    HEAP_BASE+PAGE_SECTION,
                    (uintptr_t)&__kernel_phy_end + 2*PAGE_SECTION - 1,
                    ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_UNONE);
  }

  prev_heap_end = heap_end;
  heap_end += incr;
  uint32_t n_sections = (heap_end - HEAP_BASE + PAGE_SECTION - 1) >> 20;

  if (n_sections > allocated_pages) {
    // TODO: Proper error handling
    while(1) {}
    page_list_t* res = paging_allocate(n_sections - allocated_pages);
    while (res != NULL) {
      for (int i=0;i<res->size;i++) {
        mmu_add_section(
          KERNEL_TTB_ADDRESS,
          HEAP_BASE + allocated_pages*PAGE_SECTION,
          (res->address + i)*PAGE_SECTION,
          ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_UNONE);
        allocated_pages++;
      }
      res = res->next;
    }
  } else if (n_sections < allocated_pages) {
      // TODO: Deallocate
  }

  return (uintptr_t)prev_heap_end;
}

/**	\fn int min(int const a, int const b)
 *	\param a First value.
 *	\param b Second value.
 *	\return The minimum of a and b.
 */
int min(int const a, int const b) {
    return a < b ? a : b;
}

/**	\fn int max(int const a, int const b)
 *	\param a First value.
 *	\param b Second value.
 *	\return The maximum of a and b.
 */
int max(int const a, int const b) {
    return a > b ? a : b;
}

/**	\fn int ipow(int a, int b)
 * 	\brief Computes a^b.
 *	\param a First value.
 *	\param b Second value.
 *	\return a^b
 * 	This uses a fast exponentiation algorithm.
 */
int ipow(int a, int b) {
  int result = 1;

  while (b){
    if (b&1){
      result *= a;
    }
    b >>=1 ;
    a *= a;
  }
  return result;
}
