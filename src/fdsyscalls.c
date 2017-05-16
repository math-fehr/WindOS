#include "fdsyscalls.h"
#include "scheduler.h"
#include <errno.h>
#include "syscalls.h"

inode_t fd_open_inodes[1024];
bool used[1024];

/** \fn inode_t* load_inode(inode_t val)
 *	\brief Load an inode in the file descriptor system.
 *	\param val The loaded inode.
 *	\return An unique pointer to this inode.
 * 	The goal is that every open inode should have only one representation in memory.
 *	This is in order to have it synchronized at all time.
 */
inode_t* load_inode(inode_t val) {
	int f = -1;
	for (int i=0;i<1024;i++) {
		if (used[i] && fd_open_inodes[i].st.st_ino == val.st.st_ino && fd_open_inodes[i].sb->id == val.sb->id) {
			fd_open_inodes[i].ref_count++;
			return &fd_open_inodes[i];
		} else if (!used[i]) {
			f = i;
		}
	}

	if (f == -1) {
		return NULL;
	} else {
		val.ref_count = 1;
		fd_open_inodes[f] = val;
		used[f] = true;
		return &fd_open_inodes[f];
	}
}

/** \fn bool unload_inode(inode_t val)
 *	\brief Unload an inode for the file descriptor system.
 *	\param val A pointer to the unloaded inode.
 *	\warning This pointer should be an address to a fd_open_inodes entry.
 *	\return true on success. false if this inode is not found.
 */
bool unload_inode(inode_t* val) {
	int i = ((intptr_t)val - (intptr_t)fd_open_inodes)/sizeof(inode_t);

	if (used[i]) {
		fd_open_inodes[i].ref_count--;
		if (fd_open_inodes[i].ref_count == 0) {
			used[i] = false;
		}
		return true;
	}
	return false;
}

int svc_ioctl(int fd, int cmd, int arg) {
	kdebug(D_IRQ, 2, "IOCTL %d %d %d \n", fd, cmd, arg);
	process* p = get_current_process();
	if (fd >= MAX_OPEN_FILES
	|| p->fd[fd].position < 0)
	{
		return -EBADF;
	}

	if (p->fd[fd].inode->op->ioctl == NULL) {
		return -1;
	}
	return p->fd[fd].inode->op->ioctl(*p->fd[fd].inode, cmd, arg);
}

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

	if (fd_->position >= 0 && (fd_->flags | O_WRONLY)) {
		int n = vfs_fwrite(*fd_->inode, buf, cnt, fd_->position);
		if (S_ISREG(fd_->inode->st.st_mode)) {
			fd_->position += n;
			fd_->inode->st.st_size = max(fd_->inode->st.st_size, fd_->position);
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
	unload_inode(p->fd[fd].inode);
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

	if (p->fd[fd].flags == O_WRONLY) {
		return -EBADF;
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


/** \fn uint32_t svc_getdents(uint32_t fd, struct dirent* user_entry)
 * 	\brief Explore the directory described by fd.
 *	\param fd A directory file descriptor.
 *	\param user_entry Where the entry should be written to.
 *	\return Zero on success, 1 when listing ended, -errno on error.
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



	fd_t* w_fd = &p->fd[fd];
	kdebug(D_SYSCALL, 2, "GETDENTS => %d: %d\n", fd, w_fd->inode->st.st_ino);
	if (!S_ISDIR(w_fd->inode->st.st_mode)) {
		return -ENOTDIR;
	}

	// reload dirlist
	if (w_fd->position == 0) {
		free_vfs_dir_list(w_fd->dir_entry);
		if (w_fd->inode->op == NULL) {
			return 0;
		}
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
	p->fd[i].inode = load_inode(ino);
	free(path);

	p->fd[i].flags = flags;

	if (flags & O_APPEND) {
		p->fd[i].position = ino.st.st_size;
	}

	if ((flags & O_TRUNC) && S_ISREG(ino.st.st_mode)) {
		if (p->fd[i].inode->op->resize != NULL) {
			p->fd[i].inode->op->resize(*p->fd[i].inode, 0);
			p->fd[i].inode->st.st_size = 0;
		}
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
	if (dir.op->rm == NULL) {
		return -1;
	}
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

	p->fd[i] = p->fd[oldfd];
	p->fd[i].inode->ref_count++;
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


	p->fd[newfd] = p->fd[oldfd];
	p->fd[newfd].inode->ref_count++;
	return newfd;
}
