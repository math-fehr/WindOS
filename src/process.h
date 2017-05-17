#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "vfs.h"
#include "mmu.h"
#include "memalloc.h"
#include "debug.h"
#include "stdlib.h"
#include "kernel.h"
#include <libgen.h>
#include <signal.h>
#include "../include/signals.h"

/** \struct user_context_t
 *	\brief User process data on a context switch.
 */
typedef struct {
	uint32_t cpsr; ///< Current program status register.
	uint32_t pc; ///< Return address.
	uint32_t r[15]; ///< Process registers.
} user_context_t;

/** \def MAX_OPEN_FILES
 * 	\brief Number of file descriptor that a process can open.
 */
#define MAX_OPEN_FILES 64

#define N_SIGNALS 	32

#define SIGKILL  	9
#define SIGSEGV 	11
#define SIGTERM 	15

/** \struct fd_t
 * 	\brief File descriptor structure.
 */
typedef struct {
    inode_t* inode; ///< Open file inode.
    int position; ///< Cursor position in the file.
	vfs_dir_list_t* dir_entry; ///< If the inode is a directory, store it's entries.
	int flags; ///< Open flags.
	int read_blocking;
} fd_t;

/** \enum status_process
 * 	\brief Describes status of a process
 */
typedef enum {
    status_active, ///< The process is running/runnable.
    status_wait, ///< The process is waiting for a child event.
    status_zombie, ///< Zombie mode for a killed process.
	status_blocked_svc ///< Waiting for a service call to return.
} status_process;

/** \struct wait_parameters_t
 * 	\brief Options when a parent waits for its children.
 */
typedef struct {
	pid_t 	pid; ///< The pid for the waited process. (-1 for any child)
	int*  	wstatus; ///< The pointer where we should write to on return.
} wait_parameters_t;


typedef struct {
	void     (*handler)(int);
	siginfo_t* user_siginfo;
} signal_handler_t;

/** \struct process
 *	\brief All the data representing a process.
 */
typedef struct {
    status_process status; ///< Execution status.
    int dummy; ///< Number of context switch to this process.
    uintptr_t ttb_address; ///< Address of process' translation table.
    pid_t asid; ///< Program ID
	pid_t parent_id; ///< Parent ID
    int brk; ///< Program break.
    int brk_page; ///< Number of pages allocated for program break
    fd_t fd[MAX_OPEN_FILES]; ///< Process' file descriptors
	user_context_t ctx; ///< Process' execution context.
	user_context_t old_ctx; ///< Process' execution context before a signal was caught.
	wait_parameters_t wait; ///< When in wait status, wait parameters. When in zombie status, stores exit code.
	inode_t cwd; ///< Current working directory.
	char* name; ///< Process name.
	signal_handler_t sighandlers[N_SIGNALS];
	bool allocated_framebuffer;
} process;

#define ELF_ABI_SYSTEMV 0

#define ELF_TYPE_RELOCATABLE 1
#define ELF_TYPE_EXECUTABLE 2
#define ELF_TYPE_SHARED 3
#define ELF_TYPE_CORE 4

#define ELF_MACHINE_ARM 0x28

#define ELF_FORMAT_32BIT 1
#define ELF_FORMAT_64BIT 2

/** \struct elf_header_t
 *	\brief ELF Header structure
 *
 * 	https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
 */
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

/** \struct ph_entry_t
 *	\brief Program header entry.
 */
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

/** \struct sh_entry_t
 *	\brief Section header entry.
 */
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
bool process_signal(process* p, siginfo_t signal);

#endif //PROCESS_H
