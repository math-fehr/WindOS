
#include "process.h"
#include "malloc.h"

extern unsigned int __ram_size;

process* process_load(char* path) {
    int fd = vfs_fopen(path);
    elf_header_t header;
    vfs_fread(fd,(char*)&header,sizeof(header));


    if (strncmp(header.magic_number,"\x7F""ELF",4) != 0) {
        kdebug(D_PROCESS, 2, "Can't load %s: no elf header detected.\n", path);
        return NULL;
    }

    if (header.abi != ELF_ABI_SYSTEMV) {
        kdebug(D_PROCESS, 2, "Can't load %s: wrong ABI.\n", path);
        return NULL;
    }

    if (header.type != ELF_TYPE_EXECUTABLE) {
        kdebug(D_PROCESS, 2, "Can't load %s: this is not an executable.\n", path);
        return NULL;
    }

    if (header.machine != ELF_MACHINE_ARM) {
        kdebug(D_PROCESS, 2, "Can't load %s: this isn't built for ARM.\n", path);
        return NULL;
    }

    if (header.format != ELF_FORMAT_32BIT) {
        kdebug(D_PROCESS, 2, "Can't load %s: 64bit is not supported.\n", path);
        return NULL;
    }

    uintptr_t ttb_address;

    uint32_t table_size = 16*1024 >> TTBCR_ALIGN;
    ttb_address = (uintptr_t)memalign(table_size, table_size);

    page_list_t* res = paging_allocate(2);
    if (res == NULL) {
        kdebug(D_PROCESS, 2, "Can't load %s: page allocation failed.\n", path);
        return NULL;
    }
    int section_addr = res->address*PAGE_SECTION;
    int section_stack;
    if (res->size == 1) {
        section_stack = res->next->address*PAGE_SECTION;
    } else {
        section_stack = (res->address+1)*PAGE_SECTION;
    }
    // 1MB for the program and 1MB for the stack. TODO: Make this less brutal.
    mmu_add_section(ttb_address, 0, section_addr, 0);
    kernel_printf("RS: %#010x - %#010x = %#010x => %#010x\n", __ram_size, PAGE_SECTION, __ram_size - PAGE_SECTION, section_stack);
    kernel_printf("Alignment: %#010x\n", table_size);
    mmu_add_section(ttb_address, __ram_size-PAGE_SECTION, section_stack, 0);
    // Loads executable data into memory and allocates it.
    ph_entry_t ph;
    for (int i=0; i<header.ph_num;i++) {
        int cur_pos = header.program_header_pos+i*header.ph_entry_size;
        vfs_fmove(fd, cur_pos);
        vfs_fread(fd, (char*)&ph, sizeof(ph_entry_t));
        if (ph.type == 1) {
            vfs_fmove(fd, ph.offset);
            vfs_fread(fd, (char*)(uintptr_t)(0x80000000+section_addr+ph.virtual_address), ph.file_size);
        }
    }
    /*  sh_entry_t sh;
    for (int i=0; i<header.sh_num;i++) {
    vfs_fmove(fd, header.section_header_pos+header.sh_entry_size);
    kernel_printf("f\n");
    vfs_fread(fd, (char*)&sh, sizeof(sh_entry_t));
    kernel_printf("g\n");
    if ((sh.type == 1 || sh.type == 8) && (sh.flags & 2)) {
      vfs_fmove(fd, sh.offset);
      vfs_fread(fd, (char*)(uintptr_t)(0x80000000+section_addr+sh.addr), sh.size);
    }
    }*/

    process* processus = malloc(sizeof(process));
    processus->asid = 1;
    processus->dummy = 0;
    processus->ttb_address = ttb_address;
    processus->status = status_active;
    processus->sp = __ram_size-17*sizeof(uint32_t);

    uint32_t* phy_stack = (uint32_t*)(0x80000000+section_stack+PAGE_SECTION-17*sizeof(uint32_t));
    for (int i=0;i<17;i++) {
        phy_stack[i] = 42;
    }
    phy_stack[16] =  header.entry_point;
    phy_stack[15] = 0xf0000000;
    phy_stack[14] = __ram_size;
    phy_stack[0] = 0x10; // CPSR user mode

    processus->brk = PAGE_SECTION;
    processus->brk_page = 0;

    vfs_fclose(fd);
    return processus;
}

void process_switchTo(process* p) {
    kernel_printf("TTB address: %#010x\n", p->ttb_address);
    mmu_set_ttb_0(mmu_vir2phy(p->ttb_address), TTBCR_ALIGN);
    asm volatile(   "push {r0-r12}\n"
                    "mov r1, %0\n"
                    "ldmfd r1!, {r0}\n"
                    "msr cpsr, r0\n"
                    "mov sp, r1\n"
                    "ldmfd sp, {r0-r15}\n"
                    :
                    : "r" (p->sp)
                    :);
    asm volatile(   ".globl switchTo_end\n"
                    "switchTo_end:\n"
                    "pop {r0-r12}\n" : : :);
}
