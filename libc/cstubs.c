#include "stddef.h"
#include "stdint.h"
#include "sys/types.h"
#include "sys/stat.h"
#include <fcntl.h>
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"
#include "../include/syscalls.h"
#include "../include/dirent.h"

int argc;
char **argv;
char **environ;

int _getpid() {
	int res;
	asm volatile(
				"push 	{r7}\n"
				"ldr 	r7, =#0x14\n"
				"svc 	#0\n"
				"pop	{r7}\n"
				"mov 	%0, r0\n" : "=r" (res) : :);
	return res;
}

char* get_framebuffer(int pid) {
	char* res;
	asm volatile(
				"push 	{r7}\n"
				"ldr 	r0, %1\n"
				"ldr 	r7, =#0x42\n"
				"svc 	#0\n"
				"pop	{r7}\n"
				"mov 	%0, r0\n" : "=r" (res) : "m" (pid) :);
	if ((int)(intptr_t)res < 0) {
		errno = -(int)(intptr_t)res;
		return NULL;
	}
	return res;
}


int pipe(int pipefd[2]) {
	int res;
	asm volatile(
				"push 	{r7}\n"
				"ldr 	r0, %1\n"
				"ldr 	r7, =#0x2a\n"
				"svc 	#0\n"
				"pop	{r7}\n"
				"mov 	%0, r0\n" : "=r" (res) : "m" (pipefd) :);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

int sigreturn() {
	int res;
	asm volatile(
				 "push 	{r7}\n"
				 "ldr 	r7, =#0x77\n"
				 "svc 	#0\n"
			 	 "pop 	{r7}\n"
			 	 "mov 	%0, r0\n" : "=r" (res) : :);
	errno = -res;
	return -1;
}

int sigaction(int signum, void (*handler)(int), siginfo_t* siginfo) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"ldr 	r2, %3\n"
					"ldr 	r7, =#0x43\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (signum), "m" (handler), "m" (siginfo)
					:
	);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

int _kill(pid_t pid, int sig) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"ldr 	r7, =#0x25\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (pid), "m" (sig)
					:
	);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

// 0x29
int dup(int oldfd) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r7, =#0x29\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (oldfd)
					:
	);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

// 0x3f
int dup2(int oldfd, int newfd) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"ldr 	r7, =#0x3f\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (oldfd), "m" (newfd)
					:
	);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

char *basename(char *path)
{
	char *p;
	if( path == NULL || *path == '\0' )
		return ".";
	p = path + strlen(path) - 1;
	while( *p == '/' ) {
		if( p == path )
			return path;
		*p-- = '\0';
	}
	while( p >= path && *p != '/' )
		p--;
	return p + 1;
}

char *dirname(char *path)
{
	char *p;
	if( path == NULL || *path == '\0' )
		return ".";
	p = path + strlen(path) - 1;
	while( *p == '/' ) {
		if( p == path )
			return path;
		*p-- = '\0';
	}
	while( p >= path && *p != '/' )
		p--;
	return
		p < path ? "." :
		p == path ? "/" :
		(*p = '\0', path);
}


void software_init_hook() {
	argv = 0;
	argc = 0;
	while (argv[argc] != 0) {
		argc++;
	}
	environ = (char**)((((uint32_t)(intptr_t)argv[argc-1]+strlen(argv[argc-1])+2+3)/4)*4);
}

// 0x01:
void _exit(int error_code) {
    asm volatile(   "push {r7}\n"
					"mov r7, #0x01\n"
                    "svc #0\n"
					"pop {r7}\n");
    while(1){} // wait for your death
}

int _ioctl(int fd, int mode, int arg) {
	int res;
	asm volatile(
					"push {r7}\n"
					"ldr r0, %1\n"
					"ldr r1, %2\n"
					"ldr r2, %3\n"
					"mov r7, #0x36\n"
					"svc #0\n"
					"pop {r7}\n"
					"mov %0, r0"
				:   "=r" (res)
				:   "m" (fd), "m" (mode), "m" (arg)
				:);
	if (res < 0) {
		errno = -res;
	}
	return res;
}

// 0x2d:
void *_sbrk(intptr_t increment) {
    void* res;
    asm volatile(   "ldr r0, %1\n"
					"push {r7}\n"
                    "mov r7, #0x2d\n"
                    "svc #0\n"
					"pop {r7}\n"
                    "str r0, %0\n"
                :   "=m" (res)
                :   "m" (increment)
                :);
	if (res < 0) {
		errno = -(int)(intptr_t)res;
	}
    return res;
}

// 0x02
pid_t _fork() {
	pid_t res=0;
	asm volatile(
					"push {r7}\n"
					"mov r7, #0x02\n"
					"svc #0\n"
					"pop {r7}\n"
					"mov %0, r0"
				: 	"=r" (res)
				: : );
	_sbrk(0);// À investiguer.
	return res;
}

// 0x127
int _openat(int dirfd, char* path, int flags) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"ldr 	r2, %3\n"
					"ldr 	r7, =#0x127\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (dirfd), "m" (path), "m" (flags)
					:
	);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return res;
}

//0x129
int _mknodat(int dirfd, char* path, mode_t mode, dev_t dev) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"ldr 	r2, %3\n"
					"ldr 	r3, %4\n"
					"ldr 	r7, =#0x129\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (dirfd), "m" (path), "m" (mode), "m" (dev)
					:
	);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}
int _open(char* path, int flags) {
	return _openat(AT_FDCWD, path, flags);
}

// 78
int _getdents(int fd, struct dirent* user_dirent) {
	int res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"mov 	r7, #78\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (fd), "m" (user_dirent)
					:
	);
	if (res < 0) {
		errno = -res;
	}
	return res;
}

// 0x0b:
int _execve(const char *filename, char *const argv[],
                  char *const envp[]) {
	int res;
	asm volatile(
		"push {r7}\n"
		"mov r7, #0x0b\n"
		"ldr r0, %1\n"
		"ldr r1, %2\n"
		"ldr r2, %3\n"
		"svc #0\n"
		"pop {r7}\n"
		"mov %0, r0\n"
		: "=r" (res)
		: "m" (filename), "m" (argv), "m" (envp)
		:);
	errno = -res;
	return -1;
}

int _chdir(char* path) {
	int res;
	asm volatile(
		"push {r7}\n"
		"mov r7, #0x0c\n"
		"ldr r0, %1\n"
		"svc #0\n"
		"pop {r7}\n"
		"mov %0, r0\n"
		: "=r" (res)
		: "m" (path)
		:);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}

int execvp(const char *filename, char *const argv[]) {
	char* path = getenv("PATH");
	if (path == NULL || (strchr(filename,'/') != 0) ) { // no PATH or absolute/relative path.
		return execve(filename, argv, environ); // will execute in cwd
	} else { // let's search in path
		char* tok = strtok(path, ":");
		if (tok == NULL) {
			return execve(filename, argv, environ);
		} else {
			do {
				char* dest_buf = malloc(strlen(filename)+2+strlen(tok));
				strcpy(dest_buf, tok);
				strcat(dest_buf, "/");
				strcat(dest_buf, filename);
				execve(dest_buf, argv, environ);
				free(dest_buf);
			} while ((tok = strtok(NULL, ":")) != NULL);
		}
	}
	return -1;
}

// 0xb7
char* getcwd(char* buf, size_t size) {
	char* res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"mov 	r7, #0xb7\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
					: "=r" (res)
					: "m" (buf), "m" (size)
					:
	);
	if (res < 0) {
		errno = -(int)(intptr_t)res;
	}
	return res;
}


// 0x04:
ssize_t _write(int fd, const void* buf, size_t count) {
    ssize_t res;
    asm volatile(
					"push {r7}\n"
					"ldr r0, %1\n"
                    "ldr r1, %2\n"
                    "ldr r2, %3\n"
                    "mov r7, #0x04\n"
                    "svc #0\n"
					"pop {r7}\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "m" (fd), "m" (buf), "m" (count)
                :);
	if (res < 0) {
		errno = -res;
	}
    return res;
}

// 0x06
int _close(int fd) {
    int res;
    asm volatile(
					"push {r7}\n"
					"mov r0, %1\n"
                    "mov r7, #0x06\n"
                    "svc #0\n"
					"pop {r7}\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "r" (fd)
                :);
	if (res < 0) {
		errno = -res;
	}
    return res;
}

// 0x1c
int _fstat(int fd, struct stat* buf) {
    int res;
    asm volatile(
					"push {r7}\n"
					"ldr r0, %1\n"
                    "ldr r1, %2\n"
                    "mov r7, #0x1c\n"
                    "svc #0\n"
					"pop {r7}\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "m" (fd), "m" (buf)
                :);
	if (res < 0) {
		errno = -res;
	}
    return res;
}

// 0x07
pid_t _waitpid(pid_t pid, int *wstatus, int options) {
	pid_t res;
	asm volatile(
					"push 	{r7}\n"
					"ldr 	r0, %1\n"
					"ldr 	r1, %2\n"
					"ldr 	r2, %3\n"
					"mov 	r7, #0x07\n"
					"svc 	#0\n"
					"pop 	{r7}\n"
					"mov 	%0, r0\n"
				:	"=r" (res)
				: 	"m" (pid), "m" (wstatus), "m" (options) :);
	if (res < 0) {
		errno = -res;
	}
	return res;
}

int _isatty(int fd) {
	struct stat st;
	fstat(fd, &st);

	if (S_ISCHR(st.st_mode)) {
		return 1;
	} else {
  		return 0;
	}
}

pid_t _wait(int *wstatus) {
	return _waitpid(-1, wstatus, 0);
}


// 0x13
off_t _lseek(int fd, off_t offset, int whence) {
    off_t res;
    asm volatile(
					"push {r7}\n"
					"ldr r0, %1\n"
                    "ldr r1, %2\n"
                    "ldr r2, %3\n"
                    "mov r7, #0x13\n"
                    "svc #0\n"
					"pop {r7}\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "m" (fd), "m" (offset), "m" (whence)
                :);
	if (res < 0) {
		errno = -res;
		res = -1;
	}
    return res;
}

int _unlinkat(int dfd, const char* name, int flag) {
	int res;
	asm volatile(
					"push {r7}\n"
					"ldr r0, %1\n"
					"ldr r1, %2\n"
					"ldr r2, %3\n"
					"ldr r7, =#0x12d\n"
					"svc #0\n"
					"pop {r7}\n"
					"mov %0, r0\n"
				:   "=r" (res)
				:   "m" (dfd), "m" (name), "m" (flag)
				:);
	if (res < 0) {
		errno = -res;
	}
	return res;
}

// 0x03
ssize_t _read(int fd, void* buf, size_t count) {
	ssize_t res;
    asm volatile(
					"push {r7}\n"
					"ldr r0, %1\n"
                    "ldr r1, %2\n"
                    "ldr r2, %3\n"
                    "mov r7, #0x03\n"
                    "svc #0\n"
					"pop {r7}\n"
                    "mov %0, r0\n"
                :   "=r" (res)
                :   "m" (fd), "m" (buf), "m" (count)
                :);
	if (res < 0) {
		errno = -res;
	}
    return res;
}
