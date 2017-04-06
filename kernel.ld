/******************************************************************************
*	kernel.ld
*	 by Alex Chadwick
*
*	A linker script for generation of raspberry pi kernel images.
******************************************************************************/
SECTIONS {
	/*
	* INFO: 0x10000 for QEMU and 0x8000 for raspi2
	*/
	.init 0x8000 : {
		*(.init)
	}

	__text_start = .;
	.text :
	{
	KEEP(*(.text.boot))
	*(.text)
	}
	. = ALIGN(4096);
	__text_end = .;

	__rodata_start = .;
	.rodata :
	{
	*(.rodata)
	}
	. = ALIGN(4096);
	__rodata_end = .;

	__data_start = .;
	.data :
	{
	*(.data)
	}
	. = ALIGN(4096);
	__data_end = .;

	__bss_start = .;
	.bss :
	{
	bss = .;
	*(.bss)
	}
	. = ALIGN(4096);
	__bss_end = .;
	__end = .;
}
