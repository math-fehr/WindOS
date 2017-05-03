#include "syscalls.h"



extern uint32_t current_process_id;
extern uint32_t interrupt_reg[17];
extern unsigned int __ram_size;

void svc_exit() {
	kdebug(D_IRQ, 10, "Program wants to quit (switch him to zombie state)\n");
	while(1) {}
}

uint32_t svc_sbrk(uint32_t ofs) {
	kdebug(D_IRQ, 2, "SBRK %d\n", ofs);
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
			kdebug(D_IRQ, 10, "SBRK: ENOMEM %d %d\n", pages_needed, p->brk_page);
			errno = ENOMEM;
			return -1;
		}
	} else if (pages_needed < p->brk_page) {
		// should free pages.
	}
	p->brk = p->brk + ofs;
	kdebug(D_IRQ, 2, "SBRK => %#010x\n", old_brk);
	return old_brk;
}

uint32_t svc_fork() {
	process* p 			= get_process_list()[current_process_id];
	process* copy 		= malloc(sizeof(process));
	kdebug(D_IRQ, 5, "FORK\n");
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
	copy->sp 		= p->sp - 17*4;
	copy->status 	= p->status;

	uint32_t* phy_stack = (uint32_t*)(0x80000000 + mmu_vir2phy_ttb(copy->sp, copy->ttb_address));
	for (int i=1;i<17;i++) {
		phy_stack[i] = interrupt_reg[i-1];
	}
	phy_stack[0] = interrupt_reg[16];

	int pid 		= sheduler_add_process(copy);
	kdebug(D_IRQ, 10, "FORK => %d\n", pid);
	return pid;
}

uint32_t svc_write(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_IRQ, 2, "WRITE %d %#010x %#010x\n", fd, buf, cnt);
	//int fd = r[0];
	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}

	fd_t* fd_ = &get_process_list()[current_process_id]->fd[fd];
	if (fd_->position >= 0) {
		int n = vfs_fwrite(fd_->inode, buf, cnt, 0);
		kdebug(D_IRQ, 2, "WRITE => %d\n",n);
		//fd->position += n;
		return n;
	} else {
		return -1;
	}
}

uint32_t svc_close(uint32_t fd) {
	kdebug(D_IRQ, 10, "CLOSE %d\n", fd);
	return 0;
}

uint32_t svc_fstat(uint32_t fd, struct stat* dest) {
	kdebug(D_IRQ, 2, "FSTAT %d %#010x\n", fd, dest);
	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}
	fd_t req_fd = get_process_list()[current_process_id]->fd[fd];
	if (req_fd.position >= 0) {
		memcpy(dest,&req_fd.inode->st,sizeof(struct stat));
		kdebug(D_IRQ, 2, "FSTAT OK\n");
		return 0;
	} else {
		kdebug(D_IRQ, 5, "ERR\n");
		return -1;
	}
}

uint32_t svc_read(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_IRQ, 2, "READ %d %#010x %d\n", fd, buf, cnt);
	char* w_buf = buf;
	size_t w_cnt = cnt;

	if (fd >= MAX_OPEN_FILES) {
		return -1;
	}

	fd_t* w_fd = &get_process_list()[current_process_id]->fd[fd];
	if (w_fd->position >= 0) {
		int n = vfs_fread(w_fd->inode, w_buf, w_cnt, w_fd->position);
		//w_fd->position += n;
		kdebug(D_IRQ, 2, "READ => %d\n",n);
		return n;
	} else {
		return -1;
	}
}
