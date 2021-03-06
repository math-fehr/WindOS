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


/**	\def TIMER_LOAD
 *	\brief Period of the ARM Timer.
 */
#define TIMER_LOAD 100

/** \def TTBCR_ALIGN
 * 	\brief Defines the frontier between user and kernel address space.
 *
 *	Here we choose 2GB/2GB split.
 */
#define TTBCR_ALIGN 1

/**	\def MAX_PROCESSES
 * 	\brief Maximum number of processes that can be handled
 */
#define MAX_PROCESSES 500

#define VFS_MAX_OPEN_FILES 1000
#define VFS_MAX_OPEN_INODES 1000


#endif
