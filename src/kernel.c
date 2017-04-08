#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "serial.h"
#include "timer.h"
#include "interrupts.h"
#include "gpio.h"
#include "debug.h"
#include "fat.h"
#include "storage_driver.h"

#define GPIO_LED_PIN 47

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

	serial_init();

	while(1) {
    }
	serial_write("[INFO] Ramdisk location is ");
	print_hex(__ramdisk, 4);
}
