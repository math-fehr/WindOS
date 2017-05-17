/** \file syscalls.c
 * 	\brief System calls
 *
 * 	\warning These functions are not documented as the follow the unix system
 *	call convention.
 */


#include "syscalls.h"
#include "errno.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

extern unsigned int __ram_size;

/** \fn bool his_own(process *p, void* pointer)
 * 	\brief Check if the pointer effectively points to the process' allowed userspace.
 *	\param p Checked process.
 *	\param pointer Checked pointer.
 *	\return True if this pointer refers to process' userspace, false otherwise.
 *
 *	\warning This is not fully implemented.
 *	\bug If the pointer is on a boundary, illegal access could be made as the size isn't checked.
 */
bool his_own(process *p, void* pointer) {
    (void)p;
	return (((uintptr_t)pointer) < __ram_size) && (pointer != NULL); // no further check.
}

uint32_t svc_exit(int code) {
    int current_process_id = get_current_process_id();
	kdebug(D_SYSCALL, 2, "Program %d wants to quit (switch him to zombie state)\n", current_process_id);

	int wstatus = (code << 8);
	kill_process(current_process_id, wstatus);
    process* p = get_next_process();
	if (p == NULL) {
		kdebug(D_SYSCALL, 10, "The last process has died. The end is near.\n");
		while(1) {}
	}
	kdebug(D_SYSCALL, 2, "Next process: %d\n", get_current_process_id());
	return current_process_id;
}




/*
 * Transform current process into process designed by path.
 */
uint32_t svc_execve(char* path, const char** argv, const char** envp) {
	process* p = get_current_process();

	kdebug(D_SYSCALL, 2, "EXECVE => %s\n", path);

	if ((!(his_own(p, path) && path != NULL))
	|| (!(his_own(p, argv) && argv != NULL))
	|| (!(his_own(p, envp) && envp != NULL))) {
		p->ctx.r[0] = -EFAULT;
		return p->asid;
	}

	errno = 0;
	process* new_p 		= process_load(path, p->cwd, argv, envp);
	if (new_p == NULL) {
		p->ctx.r[0] = -errno;
		return p->asid;
	}


	// Free program break.
	int n_allocated_pages = p->brk_page;
	for (int i=n_allocated_pages;i>0;i--) {
		int phy_page = mmu_vir2phy_ttb(i*PAGE_SECTION, p->ttb_address) / PAGE_SECTION; // let's hope GCC optimizes this
		paging_free(1,phy_page);
	}

	// free stack and program code
	paging_free(1,mmu_vir2phy_ttb(0, p->ttb_address)/PAGE_SECTION);


	for (int i=0;i<64;i++) {
		new_p->fd[i].position = -1;
		new_p->fd[i].dir_entry = NULL;
	}

	new_p->asid 			= p->asid;
	new_p->parent_id 		= p->parent_id;

	for (int i=0;i<64;i++) {
		new_p->fd[i].position = p->fd[i].position;
		if( new_p->fd[i].position >= 0) {
			new_p->fd[i].inode = p->fd[i].inode;
			new_p->fd[i].dir_entry = NULL;
			new_p->fd[i].flags = p->fd[i].flags;
		} else {
			new_p->fd[i].dir_entry = NULL;
			new_p->fd[i].inode = NULL;
		}
	}


	for (int i=0;i<32;i++) {
		if ((new_p->sighandlers[i].handler != SIG_DFL) && (new_p->sighandlers[i].handler != SIG_IGN)) {
			new_p->sighandlers[i].handler = SIG_DFL;
		} else {
			new_p->sighandlers[i] = p->sighandlers[i];
		}
	}

	get_process_list()[new_p->asid] = new_p;

	kdebug(D_SYSCALL, 2, "Program loaded! Freeing shit %p %p\n", p->ttb_address, p);
	free((void*)p->ttb_address);
	free(p);
	new_p->dummy = 0;
	kdebug(D_SYSCALL, 2, "EXECVE: Done\n");
	return new_p->asid;
}

char* svc_getcwd(char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "GETCWD\n");
    process* p = get_current_process();
	if (!his_own(p, buf)) {
		return (char*)-EFAULT;
	}
	return vfs_inode_to_path(p->cwd, buf, cnt);
}

uint32_t svc_chdir(char* path) {
	kdebug(D_SYSCALL, 2, "CHDIR %s\n", path);
    process* p = get_current_process();
	if (!his_own(p, path)) {
		return -EFAULT;
	}
	errno = 0;
	inode_t res = vfs_path_to_inode(&p->cwd, path);

	if (errno > 0) {
		return -errno;
	} else if (!S_ISDIR(res.st.st_mode)) {
		return -ENOTDIR;
	} else {
		p->cwd = res;
	}
	return 0;
}

uint32_t svc_sbrk(uint32_t ofs) {
	kdebug(D_SYSCALL, 2, "SBRK %d\n", ofs);
    process* p = get_current_process();
	int old_brk         = p->brk;

	int current_brk     = old_brk+ofs;
	int pages_needed    = (current_brk - 1) / (PAGE_SECTION); // As the base brk is PAGE_SECTION (should be moved though)
	if (pages_needed > 64 || pages_needed < 0) {
		return -EINVAL;
	}

	if (pages_needed > p->brk_page) {
		page_list_t* pages = paging_allocate(pages_needed - p->brk_page);
		while (pages != NULL) {
			while (pages->size > 0) {
				mmu_add_section(p->ttb_address,(p->brk_page+1)*PAGE_SECTION,pages->address*PAGE_SECTION,ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW); // TODO: setup flags
				pages->size--;
				pages->address++;
				p->brk_page++;
			}
			page_list_t* bef;
			bef = pages;
			pages = pages->next;
			free(bef);
		}
		if (pages_needed != p->brk_page) {
			kdebug(D_SYSCALL, 10, "SBRK: ENOMEM %d %d\n", pages_needed, p->brk_page);
			return -ENOMEM;
		}
	} else if (pages_needed < p->brk_page) {
		// should free pages.
	}
	p->brk = p->brk + ofs;
	kdebug(D_SYSCALL, 2, "SBRK => %#010x\n", old_brk);
	return old_brk;
}

extern uintptr_t heap_end;

uint32_t svc_fork() {
	kdebug(D_PROCESS, 2, "FORK\n");
	process* p = get_current_process();
	process* copy 		= malloc(sizeof(process));

	uint32_t table_size = 16*1024 >> TTBCR_ALIGN;
	copy->ttb_address 	= (uintptr_t)memalign(table_size, table_size);

	int pages_needed = 2+p->brk_page;
	page_list_t* res = paging_allocate(pages_needed);
	if (res == NULL || copy->ttb_address == (uintptr_t)NULL) {
		kdebug(D_PROCESS, 10, "Can't fork: page allocation failed.\n");
		return -ENOMEM;
	}

	page_list_t* prev = res;
	for (int i=0;i<pages_needed-1;i++) {
		if (res->size == 0) {
			res = res->next;
			free(prev);
			prev = res;
		}

		mmu_add_section(copy->ttb_address, i*PAGE_SECTION, res->address*PAGE_SECTION, ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW);
		// This is possible as the forked program memory space is still accessible.
		dmb();
		memcpy((void*)(intptr_t)(0x80000000 + res->address*PAGE_SECTION), (void*)(intptr_t)(i*PAGE_SECTION), PAGE_SECTION);
		dmb();
		res->size--;
		res->address++;
	}
	if (res->size == 0) {
		res = res->next;
		free(prev);
	}
	free(res);

	// Now all the data is copied..
	copy->brk 		= p->brk;
	copy->brk_page 	= p->brk_page;
	for (int i=0;i<64;i++) {
		copy->fd[i].position = p->fd[i].position;
		if( copy->fd[i].position >= 0) {
			copy->fd[i].inode = p->fd[i].inode;
			copy->fd[i].inode->ref_count++;

			copy->fd[i].dir_entry = NULL;
			copy->fd[i].flags = p->fd[i].flags;
		} else {
			copy->fd[i].dir_entry = NULL;
			copy->fd[i].inode = NULL;
		}
	}

	copy->ctx 		= p->ctx;
	copy->ctx.r[0] 	= 0;
	copy->status 	= p->status;
	copy->cwd 		= p->cwd;
	copy->dummy 	= 0;
	copy->name		= malloc(strlen(p->name)+1);
	copy->allocated_framebuffer = false;
	strcpy(copy->name, p->name);

	for (int i=0;i<32;i++) {
		copy->sighandlers[i] = p->sighandlers[i];
	}

	int pid 		= sheduler_add_process(copy);
	copy->parent_id = p->asid;
	if (pid == -1) {
		kdebug(D_SYSCALL, 5, "FORK FAILED, out of process\n");
		return -ECHILD;
	} else {
		kdebug(D_SYSCALL, 2, "FORK => %d\n", pid);
	}
	return pid;
}

pid_t svc_waitpid(pid_t pid, int* wstatus, int options) {
    (void)options;
	kdebug(D_SYSCALL, 2, "WAITPID\n");
	if (!his_own(get_current_process(), wstatus) && wstatus != NULL) {
		return -EFAULT;
	}

	int res = wait_process(get_current_process_id(), pid, wstatus);
	if (res == -1) {
		get_next_process();
	}
	return res;
}

uint32_t svc_time(time_t *tloc) {
	kdebug(D_SYSCALL, 1, "TIME\n");
	process* p = get_current_process();
	if (!his_own(p, tloc)) {
		return -EFAULT;
	}

	if (tloc == NULL) {
		return timer_get_posix_time();
	} else {
		*tloc = timer_get_posix_time();
		return *tloc;
	}
}

int svc_kill(pid_t pid, int sig) {
	process* p = get_current_process();
	int own_pid = p->asid;
	bool autokill = false;

	if (!(sig == SIGTERM || sig == SIGKILL || sig == SIGSEGV)) {
		p->ctx.r[0] = -EINVAL;
		return 0;
	}

	process** lst = get_process_list();

	siginfo_t signal;
	signal.si_signo = sig;
	signal.si_pid = own_pid;

	int wstatus = (sig << 8) | 1;

	if (pid > 0) {
		if (lst[pid] != NULL && lst[pid]->status != status_zombie) {
			p->ctx.r[0] = 0;
			if (process_signal(lst[pid], signal)) {
				kill_process(pid, wstatus);
				if (pid == own_pid) {
					autokill = true;
				}
			}
		} else {
			p->ctx.r[0] = -EINVAL;
		}
	} else if (pid == -1) {
		p->ctx.r[0] = 0;
		for (int i=1;i<MAX_PROCESSES;i++) {
			if (lst[i] != NULL && lst[i]->status != status_zombie) {
				if (process_signal(lst[i], signal)) {
					kill_process(i, wstatus);
					if (i == own_pid) {
						autokill = true;
					}
				}
			}
		}
	} else {
		p->ctx.r[0] = -ESRCH;
	}

	if (autokill) {
		get_next_process();
	}
	return 0;
}

int svc_sigaction(int signum, void (*handler)(int), siginfo_t* siginfo) {
	process* p = get_current_process();
	if (signum == SIGKILL) {
		return -EINVAL;
	} else if (signum == SIGSEGV || signum == SIGTERM) {
		p->sighandlers[signum].handler = handler;
		p->sighandlers[signum].user_siginfo = siginfo;
		return 0;
	} else {
		return -EINVAL;
	}
}

void svc_sigreturn() {
	process* p = get_current_process();
	p->ctx = p->old_ctx;
}


#include "framebuffer.h"
extern frameBuffer* kernel_framebuffer;
extern uintptr_t framebuffer_phy_addr;

/** \fn svc_getframebuffer(int target)
	\brief Remaps memory to access a target's framebuffer.
	\param target The target

	If target == -1, get the address of the output framebuffer.
	If target != -1, get program's framebuffer.
	\warning No security mechanism, we should have a permission system.
	\warning TODO: Checks.
 *//*
uint32_t svc_getframebuffer(int pid) {
	kdebug(D_SYSCALL, 20, "GetFB %d\n", pid);
	process* p = get_current_process();
	if (pid == -1) {
		intptr_t target = framebuffer_phy_addr;
		kernel_printf("Target: %p\n", target);
		int section_size = 1+(kernel_framebuffer->bufferSize >> 20); // Number of used sections.
		kernel_printf("On %d sections\n", section_size);
		intptr_t dep = 0x80000000 - (section_size << 20);
		for (int i=0;i<section_size;i++) {
			mmu_add_section(p->ttb_address, dep + i*(1 << 20), target + i*(1 << 20), 0, 0, AP_PRW_URW);
		}
		return dep;
	} else {
		if (pid == -2) {
			pid = p->asid;
		}
		process* p_target = get_process_list()[pid];
		if (p_target != NULL) {
			int section_size = 1+(kernel_framebuffer->bufferSize >> 20); // Number of used sections.
			if (!p_target->allocated_framebuffer) {
				p_target->allocated_framebuffer = true;
				page_list_t* res = paging_allocate(section_size);
				for (int i=0;i<section_size;i++) {
					mmu_add_section(
							p_target->ttb_address,
							0x80000000 - 2*(section_size << 20) + i*PAGE_SECTION,
							res->address*PAGE_SECTION,
							0, 0, AP_PRW_URW);
					res->address++;
					res->size--;

					if (res->size == 0) {
						page_list_t* tmp = res;
						res = res->next;
						free(tmp);
					}
				}
			}
			uintptr_t target = mmu_vir2phy_ttb(0x80000000 - 2*(section_size << 20), p_target->ttb_address);
			uintptr_t dep = 0x80000000 - 2*(section_size << 20);
			for (int i=0;i<section_size;i++) {
				mmu_add_section(p->ttb_address, dep + i*(1 << 20), target + i*(1 << 20), 0, 0, AP_PRW_URW);
			}
			tlb_flush_all();
			kernel_printf("GETFB %d -> %p\n", pid, dep);
			return dep;
		}
		return -ENOENT;
	}
}*/
