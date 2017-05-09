#include "syscalls.h"
#include "errno.h"


extern int current_process;
extern unsigned int __ram_size;

uint32_t svc_exit() {
	kdebug(D_SYSCALL, 2, "Program %d wants to quit (switch him to zombie state)\n", current_process);
	kill_process(current_process, 0);
	current_process = get_next_process();
	kdebug(D_SYSCALL, 2, "Next process: %d\n", current_process);
	return current_process;
}

/*
 * Transform current process into process designed by path.
 */
uint32_t svc_execve(char* path, const char** argv, const char** envp) {
	process* p 			= get_process_list()[current_process];
	errno = 0;
	process* new_p 		= process_load(path, p->cwd, argv, envp);
	if (new_p == 0) {
		p->ctx.r[0] = -errno;
		return p->asid;
	}

	// Free program break.
	int n_allocated_pages = p->brk_page;
	for (int i=n_allocated_pages;i>0;i--) {
		int phy_page = mmu_vir2phy(i*PAGE_SECTION) / PAGE_SECTION; // let's hope GCC optimizes this
		paging_free(1,phy_page);
	}

	// free stack and program code
	paging_free(1,mmu_vir2phy(0)/PAGE_SECTION);
	paging_free(1,mmu_vir2phy(__ram_size-PAGE_SECTION)/PAGE_SECTION);



	new_p->asid 			= p->asid;
	new_p->parent_id 		= p->parent_id;
	new_p->fd[0].inode		= malloc(sizeof(inode_t));
	*new_p->fd[0].inode      = vfs_path_to_inode(NULL, "/dev/serial");
	new_p->fd[0].position   = 0;
	new_p->fd[1].inode 		= malloc(sizeof(inode_t));
	*new_p->fd[1].inode      = vfs_path_to_inode(NULL, "/dev/serial");
	new_p->fd[1].position   = 0;
	new_p->fd[2].inode 		= malloc(sizeof(inode_t));
	*new_p->fd[2].inode      = vfs_path_to_inode(NULL, "/dev/serial");
	new_p->fd[2].position   = 0;

	get_process_list()[new_p->asid] = new_p;

	free((void*)p->ttb_address);
	free(p);

	return new_p->asid;
}

char* svc_getcwd(char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "GETCWD\n");
	process* p = get_process_list()[current_process];
	return vfs_inode_to_path(p->cwd, buf, cnt);
}

uint32_t svc_chdir(char* path) {
	kdebug(D_SYSCALL, 2, "CHDIR %s\n", path);
	process* p = get_process_list()[current_process];
	errno = 0;
	inode_t res = vfs_path_to_inode(&p->cwd, path);
	if (errno > 0) {
		return -errno;
	} else {
		p->cwd = res;
	}
	return 0;
}

uint32_t svc_sbrk(uint32_t ofs) {
	kdebug(D_SYSCALL, 2, "SBRK %d\n", ofs);
	process* p = get_process_list()[current_process];
	// TODO: check that the program doesn't fuck it up
	int old_brk         = p->brk;
	int current_brk     = old_brk+ofs;
	int pages_needed    = (current_brk - 1) / (PAGE_SECTION); // As the base brk is PAGE_SECTION (should be moved though)

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

uint32_t svc_fork() {
	process* p 			= get_process_list()[current_process];
	process* copy 		= malloc(sizeof(process));

	kdebug(D_SYSCALL, 2, "FORK\n");
	uint32_t table_size = 16*1024 >> TTBCR_ALIGN;
	copy->ttb_address 	= (uintptr_t)memalign(table_size, table_size);

	int pages_needed = 2+p->brk_page;
	page_list_t* res = paging_allocate(pages_needed);
	if (res == NULL) {
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
		memcpy((void*)(intptr_t)(0x80000000 + res->address*PAGE_SECTION), (void*)(intptr_t)(i*PAGE_SECTION), PAGE_SECTION);

		res->size--;
		res->address++;
	}
	if (res->size == 0) {
		res = res->next;
		free(prev);
	}
	mmu_add_section(copy->ttb_address, __ram_size-PAGE_SECTION, res->address*PAGE_SECTION, ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW);
	memcpy((void*)(intptr_t)(0x80000000 + res->address*PAGE_SECTION), (void*)(intptr_t)(__ram_size-PAGE_SECTION), PAGE_SECTION);
	free(res);

	// Now all the data is copied..
	copy->brk 		= p->brk;
	copy->brk_page 	= p->brk_page;
	for (int i=0;i<64;i++)
		copy->fd[i] = p->fd[i];

	copy->ctx 		= p->ctx;
	copy->ctx.r[0] 	= 0;
	copy->status 	= p->status;
	copy->cwd 		= p->cwd;

	int pid 		= sheduler_add_process(copy);
	copy->parent_id = p->asid;
	if (pid == -1) {
		kdebug(D_SYSCALL, 5, "FORK FAILED, out of process\n");
		return -ECHILD;
	} else {
		kdebug(D_SYSCALL, 4, "FORK => %d\n", pid);
	}
	return pid;
}

pid_t svc_waitpid(pid_t pid, int* wstatus, int options) {
	int res = wait_process(current_process, pid, wstatus);
	if (res == -1) {
		current_process = get_next_process();
	}
	return res;
}

uint32_t svc_write(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "WRITE %d %#010x %#010x\n", fd, buf, cnt);
	//int fd = r[0];
	if (fd >= MAX_OPEN_FILES) {
		return -EBADF;
	}

	fd_t* fd_ = &get_process_list()[current_process]->fd[fd];
	if (fd_->position >= 0) {
		int n = vfs_fwrite(*fd_->inode, buf, cnt, 0);
		kdebug(D_SYSCALL, 2, "WRITE => %d\n",n);
		//fd->position += n;
		if (n == -1) {
			n = -errno;
		}
		return n;
	} else {
		return -EBADF;
	}
}

uint32_t svc_close(uint32_t fd) {
	kdebug(D_SYSCALL, 2, "CLOSE %d\n", fd);
	return 0;
}

uint32_t svc_fstat(uint32_t fd, struct stat* dest) {
	kdebug(D_SYSCALL, 2, "FSTAT %d %#010x\n", fd, dest);
	if (fd >= MAX_OPEN_FILES) {
		return -EBADF;
	}
	fd_t req_fd = get_process_list()[current_process]->fd[fd];
	if (req_fd.position >= 0) {
		memcpy(dest,&req_fd.inode->st,sizeof(struct stat));
		kdebug(D_SYSCALL, 2, "FSTAT OK\n");
		return 0;
	} else {
		kdebug(D_SYSCALL, 5, "FSTAT ERR\n");
		return -EBADF;
	}
}

uint32_t svc_read(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "READ %d %#010x %d\n", fd, buf, cnt);
	char* w_buf = buf;
	size_t w_cnt = cnt;

	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}

	fd_t* w_fd = &get_process_list()[current_process]->fd[fd];
	if (w_fd->position >= 0) {
		int n = vfs_fread(*w_fd->inode, w_buf, w_cnt, w_fd->position);
		w_fd->position += n;
		kdebug(D_SYSCALL, 2, "READ => %d\n",n);
		return n;
	} else {
		return -1;
	}
}

uint32_t svc_time(time_t *tloc) {
	if (tloc == NULL) {
		return timer_get_posix_time();
	} else {
		*tloc = timer_get_posix_time();
		return *tloc;
	}
}

uint32_t svc_getdents(uint32_t fd, struct dirent* user_entry) {
	process* p = get_process_list()[current_process];
	fd_t* w_fd = &p->fd[fd];

	// reload dirlist
	if (w_fd->position == 0) {
		free_vfs_dir_list(w_fd->dir_entry);
		w_fd->dir_entry = w_fd->inode->op->read_dir(*w_fd->inode);
	}

	if (w_fd->dir_entry == NULL) {
		return -1;
	} else {
		w_fd->position++;
		user_entry->d_ino = w_fd->dir_entry->inode.st.st_ino;
		user_entry->d_type = w_fd->dir_entry->inode.st.st_mode;
		strcpy(user_entry->d_name, w_fd->dir_entry->name);
		vfs_dir_list_t *prec = w_fd->dir_entry;
		w_fd->dir_entry = w_fd->dir_entry->next;
		free(prec->name);
		free(prec);
		return 0;
	}
}

int svc_open(char* path, int flags) {
	process* p = get_process_list()[current_process];
	int i=0;
	for (;(p->fd[i].position != -1) && (i<MAX_OPEN_FILES);i++);

	if (i == MAX_OPEN_FILES) { // no available fd.
		return -1;
	}
	
	inode_t ino = vfs_path_to_inode(&p->cwd, path);

	if (errno > 0) {
		return -errno;
	} else {
		p->fd[i].position = 0;
		p->fd[i].dir_entry = NULL;
		p->fd[i].inode = malloc(sizeof(inode_t));
		*p->fd[i].inode = ino;
	}
	return i;
}
