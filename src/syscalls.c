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

uint32_t svc_exit() {
    int current_process_id = get_current_process_id();
	kdebug(D_SYSCALL, 2, "Program %d wants to quit (switch him to zombie state)\n", current_process_id);
	kill_process(current_process_id, 0);
    process* p = get_next_process();
	if (p == NULL) {
		kdebug(D_SYSCALL, 10, "The last process has died. The end is near.\n");
		while(1) {}
	}
	kdebug(D_SYSCALL, 2, "Next process: %d\n", get_current_process_id());
	return current_process_id;
}



int svc_ioctl(int fd, int cmd, int arg) {
	kdebug(D_IRQ, 2, "IOCTL %d %d %d \n", fd, cmd, arg);
	process* p = get_current_process();
	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0)
	{
		return -EBADF;
	}

	return p->fd[fd].inode->op->ioctl(*p->fd[fd].inode, cmd, arg);
}

// TODO: Refresh inodes.
off_t svc_lseek(int fd_i, off_t offset, int whence) {
	process* p = get_current_process();
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
	paging_free(1,mmu_vir2phy_ttb(__ram_size-PAGE_SECTION, p->ttb_address)/PAGE_SECTION);


	for (int i=0;i<64;i++) {
		new_p->fd[i].position = -1;
		new_p->fd[i].dir_entry = NULL;
	}

	new_p->asid 			= p->asid;
	new_p->parent_id 		= p->parent_id;

	for (int i=0;i<64;i++) {
		new_p->fd[i].position = p->fd[i].position;
		if( new_p->fd[i].position >= 0) {
			new_p->fd[i].inode = malloc(sizeof(inode_t));
			new_p->fd[i].dir_entry = NULL;
			*new_p->fd[i].inode = *p->fd[i].inode;
			new_p->fd[i].flags = p->fd[i].flags;
		} else {
			new_p->fd[i].dir_entry = NULL;
			new_p->fd[i].inode = NULL;
		}
	}

	get_process_list()[new_p->asid] = new_p;

	kdebug(D_SYSCALL, 2, "dup: Program loaded! Freeing shit %p %p\n", p->ttb_address, p);
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
	mmu_add_section(copy->ttb_address, __ram_size-PAGE_SECTION, res->address*PAGE_SECTION, ENABLE_CACHE|ENABLE_WRITE_BUFFER,0,AP_PRW_URW);
	dmb();
	memcpy((void*)(intptr_t)(0x80000000 + res->address*PAGE_SECTION), (void*)(intptr_t)(__ram_size-PAGE_SECTION), PAGE_SECTION);
	dmb();
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
	strcpy(copy->name, p->name);

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

uint32_t svc_write(uint32_t fd, char* buf, size_t cnt) {
	kdebug(D_SYSCALL, 5, "WRITE %d %#010x %#010x\n", fd, buf, cnt);
	//int fd = r[0];
	if (fd >= MAX_OPEN_FILES
        || get_current_process()->fd[fd].position < 0)
	{
		return -EBADF;
	}

	if (!his_own(get_current_process(), buf)) {
		return -EFAULT;
	}

	if (cnt == 0) {
		return 0;
	}

	fd_t* fd_ = &(get_current_process()->fd[fd]);
	if (fd_->position >= 0) {
		int n = vfs_fwrite(*fd_->inode, buf, cnt, fd_->position);
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
	process* p = get_current_process();
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
	process* p = get_current_process();
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

	process* p = get_current_process();
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
	p->status = status_active;
	p->fd[fd].position += n;
	return n;
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

/** \fn uint32_t svc_getdents(uint32_t fd, struct dirent* user_entry)
 * 	\brief Explore the directory described by fd.
 *	\param fd A directory file descriptor.
 *	\param user_entry Where the entry should be written to.
 *	\return Zero on success, -errno on error.
 */
uint32_t svc_getdents(uint32_t fd, struct dirent* user_entry) {
	process* p = get_current_process();
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

int svc_openat(int dirfd, char* path_c, int flags) {

	process* p = get_current_process();
	if (!his_own(p, path_c)) {
		return -EFAULT;
	}

	char* path = malloc(strlen(path_c)+1);
	strcpy(path,path_c);
	int i=0;
	for (;(p->fd[i].position != -1) && (i<MAX_OPEN_FILES);i++);

	if (i == MAX_OPEN_FILES) { // no available fd.
		return -ENFILE;
	}


	inode_t* base;
	if (dirfd == AT_FDCWD) {
		base = &p->cwd;
	} else {
		if ((dirfd >= 0 && dirfd < MAX_OPEN_FILES)
		 	&& p->fd[dirfd].position >= 0) {
			if (S_ISDIR(p->fd[dirfd].inode->st.st_mode)) {
				base = p->fd[dirfd].inode;
			} else {
				return -ENOTDIR;
			}
		} else {
			return -EBADF;
		}
	}
	inode_t ino = vfs_path_to_inode(base, path);

	if (errno > 0) {
		if (flags & O_CREAT) {
			char* last = path;
			while (strchr(last+1, '/') != NULL) {
				last = strchr(last+1, '/');
			}

			if (last[0] == '/') {
				last[0] = 0;
				inode_t dir = vfs_path_to_inode(base, path);
				if (errno > 0) {
					return -errno;
				}

				ino = vfs_mknod(dir, last+1, S_IFREG, 0);
				if (errno > 0) {
					return -errno;
				}
			} else {
				ino = vfs_mknod(*base, last, S_IFREG, 0);

				if (errno > 0)  {
					free(path);
					return -errno;
				}
			}
			kdebug(D_SYSCALL, 1, "Created the file as it doesn't exist.\n");
		} else {
			free(path);
			return -errno;
		}
	}

	p->fd[i].position = 0;
	p->fd[i].dir_entry = NULL;
	p->fd[i].inode = malloc(sizeof(inode_t));
	*p->fd[i].inode = ino;
	free(path);

	p->fd[i].flags = flags;

	if (flags & O_APPEND) {
		p->fd[i].position = ino.st.st_size;
	}

	if ((flags & O_TRUNC) && S_ISREG(ino.st.st_mode)) {
		p->fd[i].inode->op->resize(*p->fd[i].inode, 0);
		p->fd[i].inode->st.st_size = 0;
	}

	kdebug(D_SYSCALL,5, "OPEN => %d\n", i);
	return i;
}


int svc_unlinkat(int dirfd, char* name, int flag) {
	(void) flag;
	inode_t* base;
	process* p = get_current_process();
	if (!his_own(p, (void*)name)) {
		return -EFAULT;
	}

	if (dirfd == AT_FDCWD) {
		base = &p->cwd;
	} else {
		if ((dirfd >= 0 && dirfd < MAX_OPEN_FILES)
		 	&& p->fd[dirfd].position >= 0) {
			if (S_ISDIR(p->fd[dirfd].inode->st.st_mode)) {
				base = p->fd[dirfd].inode;
			} else {
				return -ENOTDIR;
			}
		} else {
			return -EBADF;
		}
	}

	inode_t dir = vfs_path_to_inode(base, dirname(name));
	if (errno > 0) {
		return -errno;
	}

	vfs_path_to_inode(base, name);
	if (errno > 0) {
		return -errno;
	}

	char* target = basename(name);
	dir.op->rm(dir, target);
	return -errno;
}

int svc_mknodat(int dirfd, char* pathname, mode_t mode, dev_t dev) {
	inode_t* base;
	process* p = get_current_process();
	if (!his_own(p, pathname)) {
		return -EFAULT;
	}

	if (dirfd == AT_FDCWD) {
		base = &p->cwd;
	} else {
		if ((dirfd >= 0 && dirfd < MAX_OPEN_FILES)
		 	&& p->fd[dirfd].position >= 0) {
			if (S_ISDIR(p->fd[dirfd].inode->st.st_mode)) {
				base = p->fd[dirfd].inode;
			} else {
				return -ENOTDIR;
			}
		} else {
			return -EBADF;
		}
	}

	inode_t test = vfs_path_to_inode(base, pathname);
	if (errno == 0) {
		if (S_ISDIR(test.st.st_mode)) {
			return 0;
		} else {
			return -EEXIST;
		}
	}

	char* path = malloc(strlen(pathname)+1);
	strcpy(path, pathname);
	char* last = path;
	while (strchr(last+1, '/') != NULL) {
		last = strchr(last+1, '/');
	}

	if (last[0] == '/') {
		last[0] = 0;
		inode_t dir = vfs_path_to_inode(base, path);
		if (errno > 0) {
			return -errno;
		}

		vfs_mknod(dir, last+1, mode, dev);
		if (errno > 0) {
			return -errno;
		}
	} else {
		vfs_mknod(*base, last, mode, dev);

		if (errno > 0)  {
			free(path);
			return -errno;
		}
	}
	return 0;
}

int svc_dup(int oldfd) {
	process* p = get_current_process();

	if (oldfd >= MAX_OPEN_FILES
	|| p->fd[oldfd].position < 0) {
		return -EBADF;
	}

	int i=0;
	for (;(p->fd[i].position != -1) && (i<MAX_OPEN_FILES);i++);

	if (i == MAX_OPEN_FILES) {
		return -ENFILE;
	}

	p->fd[i].inode = malloc(sizeof(inode_t));
	*p->fd[i].inode = *p->fd[oldfd].inode;
	p->fd[i].dir_entry = NULL;
	p->fd[i].flags = p->fd[oldfd].flags;
	p->fd[i].position = p->fd[oldfd].position;
	return i;
}

int svc_dup2(int oldfd, int newfd) {
	process* p = get_current_process();

	if (oldfd >= MAX_OPEN_FILES
	|| p->fd[oldfd].position < 0) {
		return -EBADF;
	}

	if (newfd >= MAX_OPEN_FILES) {
		return -EBADF;
	}

	if (p->fd[newfd].position >= 0) {
		svc_close(newfd);
	}

	p->fd[newfd].inode = malloc(sizeof(inode_t));
	*p->fd[newfd].inode = *p->fd[oldfd].inode;
	p->fd[newfd].dir_entry = NULL;
	p->fd[newfd].flags = p->fd[oldfd].flags;
	p->fd[newfd].position = p->fd[oldfd].position;
	return newfd;
}
