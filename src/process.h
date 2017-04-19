#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "vfs.h"
#include "mmu.h"
#include "memalloc.h"
#include "debug.h"
#include "stdlib.h"
/**
 * Represent an active process
 */

/**
 * status of a process
 */
typedef enum {
    status_active,
    status_free,
    status_zombie
} status_process;

/**
 * A process (running or not)
 */
typedef struct {
    status_process status;
    int dummy; //Temp value for debug
    uint32_t registers[16];
    uint32_t cpsr; // Current Program Status Register
    uintptr_t ttb_address;
    int asid; // Address Space ID
    int brk; // Program break.
    int brk_page; // Number of pages allocated for program break
} process;

#define ELF_ABI_SYSTEMV 0

#define ELF_TYPE_RELOCATABLE 1
#define ELF_TYPE_EXECUTABLE 2
#define ELF_TYPE_SHARED 3
#define ELF_TYPE_CORE 4

#define ELF_MACHINE_ARM 0x28

#define ELF_FORMAT_32BIT 1
#define ELF_FORMAT_64BIT 2

//https://en.wikipedia.org/wiki/Executable_and_Linkable_Format

typedef struct {
  char magic_number[4];
  char format;
  char endianness; // 1 => little | 2 => big
  char version;
  char abi;
  char abi_version;
  char pad[7];
  uint16_t type;
  uint16_t machine;
  uint32_t version32;
  uint32_t entry_point;
  uint32_t program_header_pos;
  uint32_t section_header_pos;
  uint32_t flags;
  uint16_t size;
  uint16_t ph_entry_size;
  uint16_t ph_num;
  uint16_t sh_entry_size;
  uint16_t sh_num;
  uint16_t sh_str_ndx;
} elf_header_t;

typedef struct {
  uint32_t type;
  uint32_t offset;
  uint32_t virtual_address;
  uint32_t physical_address;
  uint32_t file_size;
  uint32_t mem_size;
  uint32_t flags;
  uint32_t align;
} ph_entry_t;

typedef struct {
  uint32_t name;
  uint32_t type;
  uint32_t flags;
  uint32_t addr;
  uint32_t offset;
  uint32_t size;
  uint32_t link;
  uint32_t info;
  uint32_t addralign;
  uint32_t ent_size;
} sh_entry_t;

#endif //PROCESS_H
