#include "process.h"

extern unsigned int __ram_size;

process process_load(char* path) {
  int fd = vfs_fopen(path);
  elf_header_t header;
  vfs_fread(fd,(char*)&header,sizeof(header));

  process processus;
  processus.asid = 0;

  if (strncmp(header.magic_number,"\x7F""ELF",4) != 0) {
    kdebug(D_PROCESS, 2, "Can't load %s: no elf header detected.\n", path);
    return processus;
  }

  if (header.abi != ELF_ABI_SYSTEMV) {
    kdebug(D_PROCESS, 2, "Can't load %s: wrong ABI.\n", path);
    return processus;
  }

  if (header.type != ELF_TYPE_EXECUTABLE) {
    kdebug(D_PROCESS, 2, "Can't load %s: this is not an executable.\n", path);
    return processus;
  }

  if (header.machine != ELF_MACHINE_ARM) {
    kdebug(D_PROCESS, 2, "Can't load %s: this isn't build for ARM.\n", path);
    return processus;
  }

  if (header.format != ELF_FORMAT_32BIT) {
    kdebug(D_PROCESS, 2, "Can't load %s: 64bit is not supported.\n", path);
    return processus;
  }

  uintptr_t ttb_address;
  // should give a new 16kb ttb that is 16kb aligned
  ttb_address = (uintptr_t)memalign(16*1024, 16*1024);
  mmu_setup_ttb(ttb_address);
  page_list_t* res = paging_allocate(2);
  if (res == NULL) {
    kdebug(D_PROCESS, 2, "Can't load %s: page allocation failed.\n", path);
    return processus;
  }
  int section_addr = res->address*PAGE_SECTION;
  int section_stack;
  if (res->size == 1) {
    section_stack = res->next->address*PAGE_SECTION;
  } else {
    section_stack = (res->address+1)*PAGE_SECTION;
  }
  // 1MB for the program and 1MB for the stack. TODO: Make this less brutal.
  mmu_add_section(ttb_address, 0, section_addr, ENABLE_CACHE | ENABLE_WRITE_BUFFER);
  mmu_add_section(ttb_address, __ram_size-PAGE_SECTION, section_stack, ENABLE_CACHE | ENABLE_WRITE_BUFFER);
  // Loads executable data into memory and allocates it.
  ph_entry_t ph;
  for (int i=0; i<header.ph_num;i++) {
    int cur_pos = header.program_header_pos+i*header.ph_entry_size;
    vfs_fmove(fd, cur_pos);
    vfs_fread(fd, (char*)&ph, sizeof(ph_entry_t));
    if (ph.type == 1) {
      vfs_fmove(fd, ph.offset);
      vfs_fread(fd, (char*)(section_addr+ph.virtual_address), ph.file_size);
    }
  }

  sh_entry_t sh;
  for (int i=0; i<header.sh_num;i++) {
    vfs_fmove(fd, header.section_header_pos+header.sh_entry_size);
    vfs_fread(fd, (char*)&sh, sizeof(sh_entry_t));
    if ((sh.type == 1 || sh.type == 8) && (sh.flags & 2)) {
      vfs_fmove(fd, sh.offset);
      vfs_fread(fd, (char*)(section_addr+sh.addr), sh.size);
    }
  }
  processus.asid = 1;
  processus.cpsr = 0;
  processus.dummy = 0;
  processus.ttb_address = ttb_address;
  for (int i=0;i<13;i++) {
    processus.registers[i] = 0;
  }
  processus.status = status_active;

  processus.registers[15] = header.entry_point; // PC
  processus.registers[14] = header.entry_point; // LR: Exit point?
  processus.registers[13] = __ram_size-8; // SP
  processus.brk = PAGE_SECTION;
  processus.brk_page = 0;

  vfs_fclose(fd);
  return processus;
}
