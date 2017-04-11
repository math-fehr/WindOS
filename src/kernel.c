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

// Arbitrarily high adress so it doesn't conflict with something else.
// = 8MB
uint32_t __ramdisk = 0x0800000;
extern uint32_t __start;
extern void dmb();

int memory_read(uint32_t address, void* buffer, uint32_t size) {
	intptr_t base = (intptr_t)&__start + __ramdisk; // The FS is concatenated with the kernel image.
	memcpy(buffer, (void*) (address + base), size);
	dmb();

  kdebug(D_KERNEL, 1, "Disk read request at address 0x%#08x of size %d\n", address, size);
  return 0;
}

int memory_write(uint32_t address, void* buffer, uint32_t size) {
	intptr_t base = (intptr_t)&__start + __ramdisk;
  kdebug(D_KERNEL, 1, "Disk write request at address 0x%#08x of size %d\n", address, size);
	dmb();
	memcpy((void*) (address + base), buffer, size);
	return 0;
}

volatile unsigned int tim;

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

	serial_init();

	GPIO_setOutputPin(GPIO_LED_PIN);


	Timer_Setup();

	for(int i=1;i<10;i++) {
		Timer_WaitCycles(500000);
		GPIO_setPinValue(GPIO_LED_PIN, true);
		Timer_WaitCycles(500000);
		GPIO_setPinValue(GPIO_LED_PIN, false);
	}

//	enable_interrupts();
	Timer_SetLoad(1000000);
	Timer_Enable();
	Timer_Enable_Interrupts();


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
