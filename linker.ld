/**
 * The linker script
 */


SEARCH_DIR("=/usr/lib/arm-none-eabi/lib");

SECTIONS {
    /**
     * 0x8000 for the rpi, and 0x10000 for QEMU
     */
    __start = 0x8000;

    /**
     * This is the memory we let the linker allowing
     * init will contains the code which will initialize the bss and the mmu
     * kernel will contains the kernel code and the read-only data
     * data contains read/write data (.data), the bss (.bss)
     */
    MEMORY
    {
        init : org = __start, len = 1M
        kernel : org = 0xf0000000, len = 1M
        data : org = 0xc0000000, len = 1M
    }

    /**
     * The initialization of the kernel
     * This code will be computed only once
     */
    .init : {
    init.o(.text* .data* .bss* .rodata*)
    } >init

    /* We align the kernel data to 4k */
    _kernel_physical_start = ALIGN(4k);


    /**
     * We map the kernel up in the virtual memory,
     * but next to the init in the physical one
     */
    .text (_kernel_physical_start + 0xf0000000) : AT(_kernel_physical_start) {
        _kernel_virtual_start = ABSOLUTE(.);
        *(.text*)
    } >kernel


    /**
     * We map the read-only data next to the kernel code
     */
    .rodata : {
        _krodata = ABSOLUTE(.);
        *(.rodata*)
    } >kernel


    /* Adress of the end of the kernel */
    _kernel_virtual_end = .;
    /* We align the data memory to 4k*/
    _data_virtual_start = ALIGN(4k);


    /**
     * Kernel read/write data
     */
    .data : AT(_data_virtual_start - 0xf0000000) {
        _datastart = ABSOLUTE(.) ;
    *(.data) } >data

    /**
     * The kernel bss
     */
    .bss : {
        _bssstart = ABSOLUTE(.) ;
        *(.bss)
        *(COMMON)
        _bssend = ABSOLUTE(.) ;
    } >data

    _data_physical_start = _data_virtual_start - 0xf0000000;
    _bss_physical_start = _data_physical_start + (_bssstart - _datastart);
    _bss_physical_end = _data_physical_start + (_bssend - _datastart);

}
