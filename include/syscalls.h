#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include "../include/dirent.h"


char *basename(char *path);
char *dirname(char *path);
void _exit(int error_code);
int _ioctl(int fd, int mode, int arg);
void *_sbrk(intptr_t increment);
pid_t _fork();
int _openat(int dirfd, char* path, int flags);
int _mknodat(int dirfd, char* path, mode_t mode, dev_t dev);
int _open(char* path, int flags);
int _getdents(int fd, struct dirent* user_dirent);
int _execve(const char *filename, char *const argv[], char *const envp[]);
int _chdir(char* path);

char* getcwd(char* buf, size_t size);
ssize_t _write(int fd, const void* buf, size_t count);
int _close(int fd);
int _fstat(int fd, struct stat* buf);
pid_t _waitpid(pid_t pid, int *wstatus, int options);
int _isatty(int fd);
pid_t _wait(int *wstatus);
off_t _lseek(int fd, off_t offset, int whence);
int _unlinkat(int dfd, const char* name, int flag);
ssize_t _read(int fd, void* buf, size_t count);
