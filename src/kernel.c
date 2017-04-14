#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "wesh.h"

#include "serial.h"
#include "timer.h"
#include "interrupts.h"
#include "gpio.h"
#include "debug.h"
#include "ext2.h"
#include "vfs.h"
#include "storage_driver.h"
#include "process.h"
#include "scheduler.h"
#include "paging.h"

extern void start_mmu(uint32_t ttl_adress, uint32_t flags);

// Arbitrarily high adress so it doesn't conflict with something else.
// = 8MB
uint32_t __ramdisk = 0x0800000;

int memory_read(uint32_t address, void* buffer, uint32_t size) {
	uint32_t base = 0x10000 + __ramdisk; // The FS is concatenated with the kernel image.
	//TODO: don't hardcode that, because in real hardware the offset is 0x8000
	memcpy(buffer, (void*) (intptr_t) (address + base), size);
//	kernel_info("kernel.c","Disk access at address");
//	print_hex(address,2);

	return 0;
}

int memory_write(uint32_t address, void* buffer, uint32_t size) {
	uint32_t base = 0x10000 + __ramdisk;
	memcpy((void*) (intptr_t) (address + base), buffer, size);
	return 0;
}

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

    mmu_setup_mmu();
    start_mmu(TTB_ADRESS,0x00000001);

    serial_init();
	kernel_printf("[INFO][SERIAL] Serial output is hopefully ON.\n");

    /**
     * MMU TEST
     */
    mmu_add_section(0x04200000, 0x02400000, 0);
    volatile int* ptr = 0x04200042;
    volatile int* ptr2 = 0x02400042;
    *ptr = 42;
    kernel_printf("ptr : %d\nptr2 : %d \n", *ptr, *ptr2);

	int n = 5;
	setup_scheduler();
	for(int i = 0; i<n; i++) {
			create_process();
	}

	storage_driver memorydisk;
	memorydisk.read = memory_read;
	memorydisk.write = memory_write;

	superblock_t* fsroot = ext2fs_initialize(&memorydisk);

	vfs_setup();
	vfs_mount(fsroot,"/");

    wesh();
}
