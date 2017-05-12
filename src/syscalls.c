#include "syscalls.h"
#include "errno.h"
#include <sys/types.h>
#include <unistd.h>

extern int current_process_id;
extern unsigned int __ram_size;

/*
 * check if the pointer effectively points to the process' allowed userspace.
 */
bool his_own(process *p, void* pointer) {
	return (((uintptr_t)pointer) < __ram_size) && (pointer != NULL); // no further check.
}

uint32_t svc_exit() {
	kdebug(D_SYSCALL, 2, "Program %d wants to quit (switch him to zombie state)\n", current_process_id);
	kill_process(current_process_id, 0);
	current_process_id = get_next_process();
	if (current_process_id == -1) {
		kdebug(D_SYSCALL, 10, "The last process has died. The end is near.\n");
		while(1) {}
	}
	kdebug(D_SYSCALL, 2, "Next process: %d\n", current_process_id);
	return current_process_id;
}

int svc_ioctl(int fd, int cmd, int arg) {
	kdebug(D_IRQ, 2, "IOCTL %d %d %d \n", fd, cmd, arg);
	process* p = get_process_list()[current_process_id];
	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0)
	{
		return -EBADF;
	}

	return p->fd[fd].inode->op->ioctl(*p->fd[fd].inode, cmd, arg);
}

// TODO: Refresh inodes.
off_t svc_lseek(int fd_i, off_t offset, int whence) {
	process* p = get_process_list()[current_process_id];
	kdebug(D_IRQ, 2, "LSEEK %d %d %d\n", fd_i, offset, whence);

	if (fd_i >= MAX_OPEN_FILES
	|| p->fd[fd_i].position < 0)
	{
		kernel_printf("K%d\n",fd_i);
		return -EBADF;
	}

	fd_t* fd = &p->fd[fd_i];
	switch (whence) {
		case SEEK_SET:
			fd->position = offset;
			break;
		case SEEK_CUR:
			fd->position += offset;
			break;
		case SEEK_END:
			fd->position = fd->inode->st.st_size;
			break;
	}

	if (fd->position < 0) {
		fd->position = 0;
	} else if (fd->position > fd->inode->st.st_size) {
		fd->position = fd->inode->st.st_size;
	}
	kdebug(D_IRQ, 1, "LSEEK %d %d %d => %d %d\n", fd_i, offset, whence, fd->position, fd->inode->st.st_size);
	return fd->position;
}

/*
 * Transform current process into process designed by path.
 */
uint32_t svc_execve(char* path, const char** argv, const char** envp) {
	process* p 			= get_process_list()[current_process_id];

	kdebug(D_SYSCALL, 2, "EXECVE => %s\n", path);

	if (!his_own(p, path)
	|| (!his_own(p, argv) && argv != NULL)
	|| (!his_own(p, envp) && envp != NULL)) {
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
	new_p->fd[0].dir_entry 	= NULL;

	new_p->fd[1].inode 		= malloc(sizeof(inode_t));
	*new_p->fd[1].inode      = vfs_path_to_inode(NULL, "/dev/serial");
	new_p->fd[1].position   = 0;
	new_p->fd[1].dir_entry 	= NULL;

	new_p->fd[2].inode 		= malloc(sizeof(inode_t));
	*new_p->fd[2].inode      = vfs_path_to_inode(NULL, "/dev/serial");
	new_p->fd[2].position   = 0;
	new_p->fd[2].dir_entry 	= NULL;

	get_process_list()[new_p->asid] = new_p;

	kdebug(D_SYSCALL, 2, "EXECVE: Program loaded! Freeing shit %p %p\n", p->ttb_address, p);
	free((void*)p->ttb_address);
	free(p);
	new_p->dummy = 0;
	kdebug(D_SYSCALL, 2, "EXECVE: Done\n");
	return new_p->asid;
}

char* svc_getcwd(char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 2, "GETCWD\n");
	process* p = get_process_list()[current_process_id];
	if (!his_own(p, buf)) {
		return (char*)-EFAULT;
	}
	return vfs_inode_to_path(p->cwd, buf, cnt);
}

uint32_t svc_chdir(char* path) {
	kdebug(D_SYSCALL, 2, "CHDIR %s\n", path);
	process* p = get_process_list()[current_process_id];
	if (!his_own(p, path)) {
		return -EFAULT;
	}
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
	process* p = get_process_list()[current_process_id];
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
	process* p 			= get_process_list()[current_process_id];
	process* copy 		= malloc(sizeof(process));

	uint32_t table_size = 16*1024 >> TTBCR_ALIGN;
	copy->ttb_address 	= (uintptr_t)memalign(table_size, table_size);

	int pages_needed = 2+p->brk_page;
	page_list_t* res = paging_allocate(pages_needed);
	if (res == NULL || copy->ttb_address == NULL) {
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
	for (int i=0;i<64;i++) {
		copy->fd[i].position = p->fd[i].position;
		if( copy->fd[i].position >= 0) {
			copy->fd[i].inode = malloc(sizeof(inode_t));
			copy->fd[i].dir_entry = NULL;
			*copy->fd[i].inode = *p->fd[i].inode;
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
	kdebug(D_SYSCALL, 2, "WAITPID\n");
	if (!his_own(get_process_list()[current_process_id], wstatus) && wstatus != NULL) {
		return -EFAULT;
	}

	int res = wait_process(current_process_id, pid, wstatus);
	if (res == -1) {
		current_process_id = get_next_process();
	}
	return res;
}

uint32_t svc_write(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 1, "WRITE %d %#010x %#010x\n", fd, buf, cnt);
	//int fd = r[0];
	if (fd >= MAX_OPEN_FILES
	|| get_process_list()[current_process_id]->fd[fd].position < 0)
	{
		return -EBADF;
	}

	if (!his_own(get_process_list()[current_process_id], buf)) {
		return -EFAULT;
	}

	if (cnt == 0) {
		return 0;
	}

	fd_t* fd_ = &get_process_list()[current_process_id]->fd[fd];
	if (fd_->position >= 0) {
		int n = vfs_fwrite(*fd_->inode, buf, cnt, 0);
		kdebug(D_SYSCALL, 2, "WRITE => %d\n",n);
		if (S_ISREG(fd_->inode->st.st_mode)) {
			fd_->position += n;
			fd_->inode->st.st_size = fd_->position;
		}
		if (n == -1) {
			n = -errno;
		}
		return n;
	} else {
		return -EBADF;
	}
}

uint32_t svc_close(uint32_t fd) {
	process* p = get_process_list()[current_process_id];
	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0) {
		return -EBADF;
	}
	kdebug(D_SYSCALL, 2, "CLOSE %d %p\n", fd, p->fd[fd].dir_entry);

	free_vfs_dir_list(p->fd[fd].dir_entry);
	free(p->fd[fd].inode);
	p->fd[fd].dir_entry = NULL;
	p->fd[fd].inode = NULL;
	p->fd[fd].position = -1;
	return 0;
}

uint32_t svc_fstat(uint32_t fd, struct stat* dest) {
	kdebug(D_SYSCALL, 2, "FSTAT %d %#010x\n", fd, dest);
	process* p = get_process_list()[current_process_id];
	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0) {
		return -EBADF;
	}

	if (!his_own(p, dest)) {
		return -EFAULT;
	}

	*dest = p->fd[fd].inode->st;
	kdebug(D_SYSCALL, 2, "FSTAT OK\n");
	return 0;
}

uint32_t svc_read(uint32_t fd, char* buf, size_t cnt) {

	process* p = get_process_list()[current_process_id];
	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0) {
		return -EBADF;
	}

	if (!his_own(p, buf)) {
		return -EFAULT;
	}

	if (cnt == 0) {
		return 0;
	}

	int n = vfs_fread(*p->fd[fd].inode, buf, cnt, p->fd[fd].position);
	if (n == 0 && S_ISCHR(p->fd[fd].inode->st.st_mode)) {
		// block the call.
		p->status = status_blocked_svc;
		return 0;
	}
	//kdebug(D_SYSCALL, 2, "READ %d %#010x %d => %d\n", fd, buf, cnt, n);
	p->status = status_active;
	p->fd[fd].position += n;
	return n;
}

uint32_t svc_time(time_t *tloc) {
	kdebug(D_SYSCALL, 1, "TIME\n");
	process* p = get_process_list()[current_process_id];
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

uint32_t svc_getdents(uint32_t fd, struct dirent* user_entry) {
	process* p = get_process_list()[current_process_id];
	if (!his_own(p, user_entry)) {
		return -EFAULT;
	}

	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0) {
		return -EBADF;
	}

	kdebug(D_SYSCALL, 2, "GETDENTS => %d\n", fd);

	fd_t* w_fd = &p->fd[fd];

	// reload dirlist
	if (w_fd->position == 0) {
		free_vfs_dir_list(w_fd->dir_entry);
		w_fd->dir_entry = w_fd->inode->op->read_dir(*w_fd->inode);
	}

	if (w_fd->dir_entry == NULL) {
		return 1;
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
	process* p = get_process_list()[current_process_id];
	int i=0;
	for (;(p->fd[i].position != -1) && (i<MAX_OPEN_FILES);i++);

	if (i == MAX_OPEN_FILES) { // no available fd.
		return -ENFILE;
	}

	if (!his_own(p, path)) {
		return -EFAULT;
	}

	inode_t ino = vfs_path_to_inode(&p->cwd, path);
	kdebug(D_SYSCALL, 2, "OPEN => %s %d\n", path, ino.st.st_size);

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
