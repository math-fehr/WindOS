#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "vfs.h"
#include "mmu.h"
#include "memalloc.h"
#include "debug.h"
#include "stdlib.h"
#include "kernel.h"

typedef struct {
	uint32_t cpsr;
	uint32_t pc;
	uint32_t r[15];
} user_context_t;

/**
 * Represent an active process
 */

#define MAX_OPEN_FILES 64

typedef struct {
    inode_t* inode;
    int position;
	vfs_dir_list_t* dir_entry;
} fd_t;

/**
 * status of a process
 */
typedef enum {
    status_active,
    status_wait,
    status_zombie,
	status_blocked_svc
} status_process;


typedef struct {
	pid_t 	pid;
	int*  	wstatus;
} wait_parameters_t;

/**
 * A process (running or not)
 */
typedef struct {
    status_process status;
    int dummy; //Temp value for debug
    uintptr_t ttb_address;
    pid_t asid; // Address Space ID
	pid_t parent_id;
    int brk; // Program break.
    int brk_page; // Number of pages allocated for program break
    fd_t fd[MAX_OPEN_FILES];
	user_context_t ctx;
	wait_parameters_t wait; // coherent values only in wait status.
	inode_t cwd;
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

process* process_load(char* path, inode_t cwd, const char* argv[], const char *envp[]);

#endif //PROCESS_H
