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
#include "ext2.h"
#include "vfs.h"
#include "storage_driver.h"
#include "process.h"
#include "scheduler.h"

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
/*
void tree(superblock_t* fs, int inode, int depth) {
	dir_list_t* result = ext2_lsdir(fs, inode);

	while(result != 0) {
		if (result->name[0] != '.') {
			for (int i=0;i<depth;i++) {
				serial_write("| ");
			}
			if (result->attr == EXT2_ENTRY_DIRECTORY )
				serial_write("|=");
			else
				serial_write("|-");
			serial_putc('>');
			serial_write(result->name);
			serial_newline();
			if (result->attr == EXT2_ENTRY_DIRECTORY ) {
				tree(fs, result->val, depth+1);
			}
		}
		result = result->next;
	}
}*/

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	(void) r0;
	(void) r1;
	(void) atags;

	serial_init();

	kernel_printf("[INFO][SERIAL] Serial output is hopefully ON.\n");

	storage_driver memorydisk;
	memorydisk.read = memory_read;
	memorydisk.write = memory_write;


	superblock_t* fsroot = ext2fs_initialize(&memorydisk);
	if (fsroot != 0) {
		vfs_setup();
		vfs_mount(fsroot,"/");

		vfs_dir_list_t* result = vfs_readdir("/test/");

		while(result != 0) {
			perm_str_t res = vfs_permission_string(result->inode.attr);
			kernel_printf("%# 6d   %s   %s\n", result->inode.number, res.str,result->name);
			result = result->next;
		}
		//tree(fsroot, fsroot->root, 0);
	}

    test_scheduler();
	while(1) {
    }
}
