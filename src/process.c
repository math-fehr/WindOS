
#include "process.h"
#include "malloc.h"
#include "errno.h"

extern unsigned int __ram_size;

/** \fn process* process_load(char* path, inode_t cwd, const char* argv[], const char* envp[])
 * 	\brief Loads a process into memory and creates its data structure.
 *	\param path Path to load.
 * 	\param cwd If the path is relative, it is relative to cwd.
 * 	\param argv Argument table passed to this program.
 *	\param envp Environment table passed to this program.
 *	\return The newly created process on success. NULL on failure.
 *
 *	This function:
 *	- find the file.
 * 	- parse ELF header.
 *  - allocate memory for the process and set up program's translation table.
 * 	- allocate and fill process data structure. 
 */
process* process_load(char* path, inode_t cwd, const char* argv[], const char* envp[]) {
    inode_t fd = vfs_path_to_inode(&cwd, path);

    if (errno > 0) {
        kdebug(D_PROCESS, 4, "Could not load %s: %s\n", path, strerror(errno));
		errno = ENOENT;
        return 0;
    }
    elf_header_t header;
   	vfs_fread(fd,(char*)&header,sizeof(elf_header_t),0);

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
	if (ttb_address == 0) {
		int * test = malloc(50);
		kdebug(D_PROCESS, 10, "Something failed and this is important %p. %s\n", test, strerror(errno));
		errno = ENOMEM;
		return NULL;
	}
    page_list_t* res = paging_allocate(2);
    if (res == NULL) {
        kdebug(D_PROCESS, 10, "Can't load %s: page allocation failed.\n", path);
		errno = ENOMEM;
        return NULL;
    }
    int section_addr = res->address*PAGE_SECTION;
    int section_stack;
    if (res->size == 1) {
        section_stack = res->next->address*PAGE_SECTION;
		free(res->next);
		free(res);
    } else {
        section_stack = (res->address+1)*PAGE_SECTION;
		free(res);
    }
    // 1MB for the program. TODO: Make this less brutal. (not hardcoded as i could read the symbol table)
    mmu_add_section(ttb_address, 0, section_addr, ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW);
    mmu_add_section(ttb_address, __ram_size-PAGE_SECTION, section_stack, ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW);
    // Loads executable data into memory and allocates it.
    ph_entry_t ph;
    for (int i=0; i<header.ph_num;i++) {
        int cur_pos = header.program_header_pos+i*header.ph_entry_size;
        vfs_fread(fd, (char*)&ph, sizeof(ph_entry_t), cur_pos);
        if (ph.type == 1) {
            vfs_fread(fd, (char*)(uintptr_t)(0x80000000+section_addr+ph.virtual_address), ph.file_size, ph.offset);
        }
    }
	// TODO: Dynamic linking?
    sh_entry_t sh;
    for (int i=0; i<header.sh_num;i++) {
	    vfs_fread(fd, (char*)&sh, sizeof(sh_entry_t), header.section_header_pos+i*header.sh_entry_size);

	  /* if ((sh.type == 1 || sh.type == 8) && (sh.flags & 2)) {
	      vfs_fmove(fd, sh.offset);
	      vfs_fread(fd, (char*)(uintptr_t)(0x80000000+section_addr+sh.addr), sh.size);
	  }*/
	  	if (sh.type == 2) { // Symbol table
			//kernel_printf("SYMTABLE\n");
		} else if (sh.type == 3) { // String table
		//	kernel_printf("STRTABLE\n");
		}
    }

    uint32_t* phy_base = (uint32_t*)(intptr_t)(0x80000000+section_addr);
	int position = 0;
	int argc = 0;
	if (argv != NULL) {
		while (argv[argc] != 0) {
			argc++;
		}

		position += 4*(argc + 1);

		for (int i=0;i<argc;i++) {
			phy_base[i] = position;
			strcpy((char*)(intptr_t)(position+0x80000000+section_addr), argv[i]);
			position += strlen(argv[i]) + 1;
		}
		phy_base[argc] = 0;
	}

	int ofs =(position+3)/4;
	position = 4*ofs;
	if (envp != NULL) {
		int envc = 0;
		while(envp[envc]) {
			envc++;
		}

		position += 4*(envc + 1);
		for (int i=0;i<envc;i++) {
			phy_base[ofs+i] = position;
			strcpy((char*)(intptr_t)(position+0x80000000+section_addr), envp[i]);
			position += strlen(envp[i]) + 1;
		}
		phy_base[ofs+envc]=0;
	}

    process* processus = malloc(sizeof(process));
    processus->asid = 1;
    processus->dummy = 0;
    processus->ttb_address = ttb_address;
    processus->status = status_active;
	processus->ctx.cpsr = 0x110;
	processus->ctx.pc = header.entry_point;
	for (int i=0;i<15;i++) {
		processus->ctx.r[i] = i;
	}

    processus->brk = PAGE_SECTION;
    processus->brk_page = 0;
	processus->cwd = cwd;

	kdebug(D_PROCESS, 2, "Program loaded %s. S0=%p, S1=%p\n ttb=%p\n", path, section_addr, section_stack, ttb_address);


    for (int i=0;i<MAX_OPEN_FILES;i++) {
        processus->fd[i].position = -1;
    }

    return processus;
}
