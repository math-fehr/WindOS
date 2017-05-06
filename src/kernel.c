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
#include "mmu.h"
#include "dev.h"

#include "malloc.h"


extern void start_mmu(uint32_t ttl_address, uint32_t flags);

extern uint32_t __ramfs_start;
extern void dmb();

extern uint32_t current_process_id;

int memory_read(uint32_t address, void* buffer, uint32_t size) {
    kdebug(D_KERNEL, 0, "Disk read  request at address %#08x of size %d\n", address, size);

    dmb();
	intptr_t base = (intptr_t)&__ramfs_start; // The FS is concatenated with the kernel image.
	memcpy(buffer, (void*) (address + base), size);

    return 0;
}

int memory_write(uint32_t address, void* buffer, uint32_t size) {
	intptr_t base = (intptr_t)&__ramfs_start;
    kdebug(D_KERNEL, 0, "Disk write request at address %#08x of size %d\n", address, size);
	dmb();
	memcpy((void*) (address + base), buffer, size);
	return 0;
}

volatile unsigned int tim;

volatile uint32_t __ram_size;

extern char __kernel_bss_start;
extern char __kernel_bss_end;

extern int __kernel_phy_end;

void blink(int n) {
	GPIO_setPinValue(GPIO_LED_PIN, true);
	Timer_WaitCycles(1000000);
	GPIO_setPinValue(GPIO_LED_PIN, false);
	Timer_WaitCycles(1000000);
	for(int i=0;i<n;i++) {
		GPIO_setPinValue(GPIO_LED_PIN, true);
		Timer_WaitCycles(250000);
		GPIO_setPinValue(GPIO_LED_PIN, false);
		Timer_WaitCycles(250000);
	}
	Timer_WaitCycles(750000);
	GPIO_setPinValue(GPIO_LED_PIN, true);
}


void kernel_main(uint32_t memory) {
	// Initialize BSS segment
	for (char* val = (char*)&__kernel_bss_start; val < (char*)&__kernel_bss_end; val++) {
		(*val)=0;
	}

    current_process_id = -1;

	__ram_size = memory;
	GPIO_setOutputPin(GPIO_LED_PIN);

	// Remove linear mapping of the first 2GB
	for (uint32_t i=0;i<0x80000000;i+=PAGE_SECTION) {
		mmu_delete_section(0xf0004000, i);
	}
    blink(5);


	serial_init();

	// TTB1 is already set up on boot (-> 0x4000)
	// TTB0 is set up on each context switch
	mmu_setup_ttbcr(TTBCR_ALIGN);

	paging_init(__ram_size >> 20, 1+((((uintptr_t)&__kernel_phy_end) + PAGE_SECTION - 1) >> 20));

	kernel_printf("[INFO][SERIAL] Serial output is hopefully ON.\n");

	Timer_Setup();
	enable_interrupts();
	Timer_SetLoad(30000);


	storage_driver memorydisk;
	memorydisk.read    = memory_read;
	memorydisk.write   = memory_write;

	superblock_t* fsroot = ext2fs_initialize(&memorydisk);
    superblock_t* devroot = dev_initialize(10);

	vfs_setup();
	vfs_mount(fsroot,"/");
    vfs_mkdir("/","dev",0x1FF);
    vfs_mount(devroot,"/dev");


	setup_scheduler();
	const char* param[] = {"Bonjour", "Au revoir", 0};

	process* p = process_load("/bin/wesh", param, NULL); // init program
	p->fd[0].inode      = vfs_path_to_inode("/dev/serial");
	p->fd[0].position   = 0;
	p->fd[1].inode      = vfs_path_to_inode("/dev/serial");
	p->fd[1].position   = 0;

	if (p != NULL) {
		sheduler_add_process(p);
		Timer_Enable();
		Timer_Enable_Interrupts();
		process_switchTo(p);
	} else {
		kdebug(D_KERNEL, 10, "[ERROR] Could not load init");
		while(1) {}
	}
}
