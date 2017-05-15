#ifndef KERNEL_H
#define KERNEL_H

/** \def PERIPHERALS_BASE
 * 	\brief Virtual base address for Raspberry Pi peripherals
 */
#define PERIPHERALS_BASE  0xbf000000
#define GPU_IO_BASE       0x7E000000
#define GPU_CACHED_BASE   0x40000000
#define GPU_UNCACHED_BASE 0xC0000000

#ifndef RPI2
  #define GPU_MEM_BASE	GPU_UNCACHED_BASE
#else
  #ifdef GPU_L2_CACHE_ENABLED
    #define GPU_MEM_BASE	GPU_CACHED_BASE
  #else
    #define GPU_MEM_BASE	GPU_UNCACHED_BASE
  #endif
#endif

#define TIMER_LOAD 500

/** \def TTBCR_ALIGN
 * 	\brief Defines the frontier between user and kernel address space.
 *
 *	Here we choose 1GB/3GB split.
 */
#define TTBCR_ALIGN TTBCR_ALIGN_4K

/**	\def MAX_PROCESSES
 * 	\brief Maximum number of processes that can be handled
 */
#define MAX_PROCESSES 500

#define VFS_MAX_OPEN_FILES 1000
#define VFS_MAX_OPEN_INODES 1000


#endif
