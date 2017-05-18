/** \file kernel.c
 *	\brief Kernel entry point
 *
 * 	One goal: set up devices and peripherals before launching the first program.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

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
#include "mailbox.h"
#include "kernel.h"
#include "procfs.h"
#include "fdsyscalls.h"
#include "framebuffer.h"
#include "fb.h"

extern void start_mmu(uint32_t ttl_address, uint32_t flags);

/** \var extern uint32_t __ramfs_start
 * 	\brief Start of the in-RAM filesystem, as defined by the linker script.
 */
extern uint32_t __ramfs_start;

/** \var volatile uint32_t __ram_size
 * 	\brief Total available RAM
 * 	This is set up in initsys.c by reading the ATAGS.
 */
volatile uint32_t __ram_size;

extern char __kernel_bss_start;
extern char __kernel_bss_end;

extern int __kernel_phy_end;

/**	\fn int memory_read(uint32_t address, void* buffer, uint32_t size)
 *	\brief Read driver for the in-memory filesystem.
 *	\param address Ramdisk offset to read.
 * 	\param buffer Destination buffer.
 *	\param size Size to read.
 *	\return 0
 */
int memory_read(uint32_t address, void* buffer, uint32_t size) {
    kdebug(D_KERNEL, 0, "Disk read  request at address %p of size %d to %p\n", address, size, buffer);

    dmb();
	intptr_t base = (intptr_t)&__ramfs_start; // The FS is concatenated with the kernel image.
	memcpy(buffer, (void*) (address + base), size);

    return 0;
}

/**	\fn int memory_write(uint32_t address, void* buffer, uint32_t size)
 *	\brief Write driver for the in-memory filesystem.
 *	\param address Ramdisk offset to write.
 * 	\param buffer Source buffer.
 *	\param size Size to write.
 *	\return 0
 */
int memory_write(uint32_t address, void* buffer, uint32_t size) {
	intptr_t base = (intptr_t)&__ramfs_start;
    kdebug(D_KERNEL, 0, "Disk write request at address %#08x of size %d\n", address, size);
	dmb();
	memcpy((void*) (address + base), buffer, size);
	return 0;
}

/** \fn void blink(int n)
 * 	\brief ACT LED blinking
 *	\param n The number of blinks.
 */
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



//We use a buffer to store the frameBuffer informations
frameBuffer* kernel_framebuffer;
uintptr_t framebuffer_phy_addr;

/** \fn void kernel_main(uint32_t memory)
 * 	\brief Kernel main function.
 *
 *	Called by initsys.c, this function initialize high level kernel features.
 * 	It:
 * 	- clears the BSS segment.
 *	- Remove the linear mapping on kernel's TTB.
 * 	- Initialize serial output, scheduler, page allocation, USPi library.
 *	- Initialize filesystem features.
 *  - Set up the timer.
 *	- Load the 'init' process and context switch.
 */
void kernel_main(uint32_t memory) {
	// Initialize BSS segment
	for (char* val = (char*)&__kernel_bss_start; val < (char*)&__kernel_bss_end; val++) {
		(*val)=0;
	}

	__ram_size = memory;
	GPIO_setOutputPin(GPIO_LED_PIN);

	// Remove linear mapping of the first 2GB
	for (uint32_t i=0;i<0x80000000;i+=PAGE_SECTION) {
		mmu_delete_section(0xf0004000, i);
	}

    serial_init();
	serial2_init();
    setup_scheduler();

    enable_interrupts();


	// TTB1 is already set up on boot (-> 0x4000)
	// TTB0 is set up on each context switch
	mmu_setup_ttbcr(TTBCR_ALIGN);

	paging_init((__ram_size >> 20) - 1, 2+((((uintptr_t)&__kernel_phy_end) + PAGE_SECTION - 1) >> 20));

	kernel_printf("[INFO][SERIAL] Serial output is hopefully ON.\r");


    unsigned char mac[6];
    mbxGetMACAddress(mac);
    kernel_printf("Mac address : %X:%X:%X:%X:%X:%X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);


	asm volatile("mrs r0,cpsr\n"
				 "orr r0,r0,#0x80\n"
				 "msr cpsr_c,r0\n");
	Timer_Setup();
	Timer_SetLoad(TIMER_LOAD);
	Timer_Enable();


	kernel_framebuffer = memalign(16,sizeof(frameBuffer));

    if(fb_initialize(kernel_framebuffer,1024,768,24,1+MAX_WINDOWS*768) == FB_SUCCESS) {
        kernel_printf("Frame Buffer was correctly initialized\n");
    }
	kernel_printf("Ramsize: %x\n", __ram_size);
	kernel_printf("B: %p\n", kernel_framebuffer->bufferPtr);
	kernel_printf("S: %x\n", kernel_framebuffer->bufferPtr >> 20);
	kernel_framebuffer->bufferPtr &= ~0xC0000000;
	framebuffer_phy_addr = kernel_framebuffer->bufferPtr;
    uintptr_t section_to = (uintptr_t)kernel_framebuffer->bufferPtr >> 20;
	int taille_sections = (1+MAX_WINDOWS*kernel_framebuffer->bufferSize) >> 20;

	for (int i=0;i<taille_sections;i++) {
    	mmu_add_section(0xf0004000,0xFFF00000-(taille_sections << 20)+i*(1<<20),(section_to << 20)+i*(1<<20),0,0,AP_PRW_URW);
	}

    kernel_framebuffer->bufferPtr = (kernel_framebuffer->bufferPtr & 0x000FFFFF) | (0xFFF00000-(taille_sections << 20));
	kernel_printf("V: %x\n", kernel_framebuffer->bufferPtr);

    volatile uint16_t* frame = (volatile uint16_t*)(intptr_t)kernel_framebuffer->bufferPtr;
	kernel_printf("SZ: %d\n", kernel_framebuffer->bufferSize);
	kernel_printf("%dx%d\n", kernel_framebuffer->width, kernel_framebuffer->height);
	kernel_printf("%dx%d\n", kernel_framebuffer->virtualWidth, kernel_framebuffer->virtualHeight);
	kernel_printf("P: %d\n", kernel_framebuffer->pitch);
	char* pos = kernel_framebuffer->bufferPtr;

	int n_1 = Timer_GetTime();
	int n_lines = 0;
    for(int i = 0; i<kernel_framebuffer->bufferSize; i+=3) {
		int r,g,b;
		int j = (i - 17*n_lines) % 255;
		if (j < 42) {
			r = 255;
			g = j*6;
			b = 0;
		} else if (j < 2*42) {
			r = 255+(42-j)*6;
			g = 255;
			b = 0;
		} else if (j < 3*42) {
			r = 0;
			g = 255;
			b = (j-2*42)*6;
		} else if (j < 4*42) {
			r = 0;
			g = 255+(3*42-j)*6;
			b = 255;
		} else if (j < 5*42) {
			r = (j-4*42)*6;
			g = 0;
			b = 255;
		} else {
			r = 255;
			g = 0;
			b = 255 + (5*42-j)*6;
		}
		((uint8_t*) kernel_framebuffer->bufferSize+kernel_framebuffer->bufferPtr)[i] = r;
		((uint8_t*) kernel_framebuffer->bufferSize+kernel_framebuffer->bufferPtr)[i+1] = g;
		((uint8_t*) kernel_framebuffer->bufferSize+kernel_framebuffer->bufferPtr)[i+2] = b;

		if (i % kernel_framebuffer->pitch == 0) {
			n_lines++;
		}
    }

	memcpy(kernel_framebuffer->bufferPtr, kernel_framebuffer->bufferPtr+kernel_framebuffer->bufferSize, kernel_framebuffer->bufferSize);

	kernel_printf("Buffer|position: %p\n", kernel_framebuffer->bufferPtr);
	kernel_printf("Timer: %d\n", Timer_GetTime() - n_1);

	dmb();
    //USPiInitialize();


	storage_driver memorydisk;
	memorydisk.read    = memory_read;
	memorydisk.write   = memory_write;


	superblock_t* fsroot = ext2fs_initialize(&memorydisk);
	if (fsroot == NULL) {
        kernel_printf("Error while initializing fsroot\n");
		while(1) {}
	}
	superblock_t* devroot = dev_initialize(10);
	if (devroot == NULL) {
        kernel_printf("Error while initializing devroot\n");
		while(1) {}
	}

	superblock_t* procroot = proc_initialize(20);
	if (procroot == NULL) {
		kernel_printf("Error while initializing procroot\n");
		while(1) {}
	}


	vfs_setup();
	vfs_mount(fsroot,"/");


    vfs_mkdir("/","dev",0x1FF);
	vfs_mount(devroot,"/dev");


    vfs_mkdir("/","proc",0x1FF);
    vfs_mount(procroot,"/proc");


	const char* param[] = {"/bin/init",0};
    const char* env[] = {NULL};

	process* p = process_load("/bin/init", vfs_path_to_inode(NULL, "/"), param, env); // init program

	p->fd[0].inode 		= load_inode(vfs_path_to_inode(NULL, "/dev/serial2")); // creates an unique inode for serial.
	p->fd[0].position   = 0;

	p->fd[1].inode      = load_inode(vfs_path_to_inode(NULL, "/dev/serial2"));
	p->fd[1].position   = 0;


	pipe_init();
	fb_init();

	if (p != NULL) {

		sheduler_add_process(p);
        get_next_process();

	    RPI_GetIRQController()->Enable_Basic_IRQs = RPI_BASIC_ARM_TIMER_IRQ;
		dmb();


		p->parent_id 	= p->asid; // (Badass process)
		kernel_printf("%p\n", p);
		kernel_printf("%p %p\n", p->ttb_address, mmu_vir2phy(p->ttb_address));
	    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
		asm volatile(
			"mov 	r0, %0\n"
			"ldmfd 	r0!, {r1, lr}\n"
			"msr 	spsr, r1\n"
			"ldmfd 	r0, {r0-r14}^\n"
			"movs 	pc, lr\n"
			:
			: "r" (&p->ctx)
			:);

	} else {
		kdebug(D_KERNEL, 10, "[ERROR] Could not load init");
		while(1) {}
	}
}
