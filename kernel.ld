SEARCH_DIR("=/usr/lib/arm-none-eabi/lib");

SECTIONS {
	/* Loader section that boots and setup the mmu before going into virtual
	 * addressing
	 */
	 . = 0x8000;
	 __start = .;
	.init : {
		build/loader.o(.text* .data* .bss* .rodata*)
	}
	.initsys : {
		build/initsys.o(.text* .data* .bss* .rodata*)
	}


	__kernel_phy_start = ALIGN(4k);
	.text (__kernel_phy_start + 0xf0000000) : AT(__kernel_phy_start) {
		*(.text*)
	}
	. = ALIGN(4K);
	.rodata : {
		*(.rodata*)
	}
	. = ALIGN(4K);
	.data : {
		*(.data)
	}
	. = ALIGN(4K);
	.bss : {
		__kernel_bss_start = ABSOLUTE(.);
		*(.bss)
		*(COMMON)
		__kernel_bss_end = ABSOLUTE(.);
	}
	. = ALIGN(4K);
	__ramfs_start = .;
	.fs : {
		build/fs.img(.data*)
	}
	__kernel_phy_end = . - 0xf0000000;

}
