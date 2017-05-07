#ifndef KERNEL_H
#define KERNEL_H

#include "mmu.h"

#define PERIPHERALS_BASE 0xbf000000UL

#define TTBCR_ALIGN TTBCR_ALIGN_4K

/**
 * Maximum number of processes that can be handled
 */
#define MAX_PROCESSES 500

#define VFS_MAX_OPEN_FILES 1000
#define VFS_MAX_OPEN_INODES 1000

#endif
