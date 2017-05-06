#include "syscalls.h"



extern uint32_t current_process_id;
extern uint32_t interrupt_reg[17];
extern unsigned int __ram_size;

uint32_t svc_exit() {
	kdebug(D_SYSCALL, 10, "Program %d wants to quit (switch him to zombie state)\n", current_process_id);
	get_process_list()[current_process_id]->status = status_zombie;
	current_process_id = get_next_process();
	return current_process_id;
}

uint32_t svc_execve(char* path, const char** argv, const char** envp) {
	process* p 			= get_process_list()[current_process_id];

	// Free program break.
	int n_allocated_pages = p->brk_page;
	for (int i=n_allocated_pages;i>0;i--) {
		int phy_page = mmu_vir2phy(i*PAGE_SECTION) / PAGE_SECTION; // let's hope GCC optimizes this
		paging_free(phy_page,1);
	}

	// free stack and program code
	paging_free(mmu_vir2phy(0)/PAGE_SECTION,1);
	paging_free(mmu_vir2phy(__ram_size-PAGE_SECTION)/PAGE_SECTION,1);

	process* new_p = process_load(path,argv,envp);


	new_p->asid = p->asid;
	new_p->fd[0].inode      = vfs_path_to_inode("/dev/serial");
	new_p->fd[0].position   = 0;
	new_p->fd[1].inode      = vfs_path_to_inode("/dev/serial");
	new_p->fd[1].position   = 0;

	get_process_list()[new_p->asid] = new_p;

	free((void*)p->ttb_address);
	free(p);

	return new_p->asid;
}

uint32_t svc_sbrk(uint32_t ofs) {
	kdebug(D_SYSCALL, 2, "SBRK %d\n", ofs);
	process* p = get_process_list()[current_process_id];
	// TODO: check that the program doesn't fuck it up
	int old_brk         = p->brk;
	int current_brk     = old_brk+ofs;
	int pages_needed    = (current_brk - 1) / (PAGE_SECTION); // As the base brk is PAGE_SECTION (should be moved though)

	if (pages_needed > p->brk_page) {
		page_list_t* pages = paging_allocate(pages_needed - p->brk_page);
		while (pages != NULL) {
			while (pages->size > 0) {
				mmu_add_section(p->ttb_address,(p->brk_page+1)*PAGE_SECTION,pages->address*PAGE_SECTION,0); // TODO: setup flags
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
			errno = ENOMEM;
			return -1;
		}
	} else if (pages_needed < p->brk_page) {
		// should free pages.
	}
	p->brk = p->brk + ofs;
	kdebug(D_SYSCALL, 2, "SBRK => %#010x\n", old_brk);
	return old_brk;
}

uint32_t svc_fork() {
	process* p 			= get_process_list()[current_process_id];
	process* copy 		= malloc(sizeof(process));

	kdebug(D_SYSCALL, 2, "FORK\n");
	uint32_t table_size = 16*1024 >> TTBCR_ALIGN;
	copy->ttb_address 	= (uintptr_t)memalign(table_size, table_size);

	int pages_needed = 2+p->brk_page;
	page_list_t* res = paging_allocate(pages_needed);
	if (res == NULL) {
		kdebug(D_PROCESS, 10, "Can't fork: page allocation failed.\n");
		return -1;
	}

	page_list_t* prev = res;
	for (int i=0;i<pages_needed-1;i++) {
		if (res->size == 0) {
			res = res->next;
			free(prev);
			prev = res;
		}

		mmu_add_section(copy->ttb_address, i*PAGE_SECTION, res->address*PAGE_SECTION, 0);
		// This is possible as the forked program memory space is still accessible.
		memcpy((void*)(intptr_t)(0x80000000 + res->address*PAGE_SECTION), (void*)(intptr_t)(i*PAGE_SECTION), PAGE_SECTION);

		res->size--;
		res->address++;
	}
	if (res->size == 0) {
		res = res->next;
		free(prev);
	}
	mmu_add_section(copy->ttb_address, __ram_size-PAGE_SECTION, res->address*PAGE_SECTION, 0);
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

	int pid 		= sheduler_add_process(copy);
	if (pid == -1) {
		kdebug(D_SYSCALL, 5, "FORK FAILED, out of process\n");
	} else {
		kdebug(D_SYSCALL, 4, "FORK => %d\n", pid);
	}
	return pid;
}

uint32_t svc_write(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "WRITE %d %#010x %#010x\n", fd, buf, cnt);
	//int fd = r[0];
	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}

	fd_t* fd_ = &get_process_list()[current_process_id]->fd[fd];
	if (fd_->position >= 0) {
		int n = vfs_fwrite(fd_->inode, buf, cnt, 0);
		kdebug(D_SYSCALL, 2, "WRITE => %d\n",n);
		//fd->position += n;
		return n;
	} else {
		return -1;
	}
}

uint32_t svc_close(uint32_t fd) {
	kdebug(D_SYSCALL, 10, "CLOSE %d\n", fd);
	return 0;
}

uint32_t svc_fstat(uint32_t fd, struct stat* dest) {
	kdebug(D_SYSCALL, 2, "FSTAT %d %#010x\n", fd, dest);
	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}
	fd_t req_fd = get_process_list()[current_process_id]->fd[fd];
	if (req_fd.position >= 0) {
		memcpy(dest,&req_fd.inode->st,sizeof(struct stat));
		kdebug(D_SYSCALL, 2, "FSTAT OK\n");
		return 0;
	} else {
		kdebug(D_SYSCALL, 5, "FSTAT ERR\n");
		return -1;
	}
}

uint32_t svc_read(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "READ %d %#010x %d\n", fd, buf, cnt);
	char* w_buf = buf;
	size_t w_cnt = cnt;

	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}

	fd_t* w_fd = &get_process_list()[current_process_id]->fd[fd];
	if (w_fd->position >= 0) {
		int n = vfs_fread(w_fd->inode, w_buf, w_cnt, w_fd->position);
		//w_fd->position += n;
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
