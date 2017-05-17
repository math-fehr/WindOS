#ifndef FDSYSCALLS_H
#define FDSYSCALLS_H

#include "debug.h"
#include "process.h"
#include "pipefs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../include/dirent.h"
 

inode_t* load_inode(inode_t val);
bool unload_inode(inode_t* val);

uint32_t svc_write(uint32_t fd, char* buf, size_t cnt);
uint32_t svc_close(uint32_t fd);
uint32_t svc_fstat(uint32_t fd, struct stat* dest);
uint32_t svc_read(uint32_t fd, char* buf, size_t cnt);

off_t 	 svc_lseek(int fd, off_t offset, int whence);
int 	 svc_openat(int dirfd, char* path, int flags);
int 	 svc_mknodat(int dirfd, char* path, mode_t mode, dev_t flags);
int 	 svc_unlinkat(int dfd, char* name, int flag);
int 	 svc_ioctl(int fd, int cmd, int arg);

int 	 svc_dup(int oldfd);
int 	 svc_dup2(int oldfd, int newfd);
int 	 svc_pipe(int pipefd[2]);

uint32_t svc_getdents(uint32_t fd, struct dirent* user_entry);
#endif
