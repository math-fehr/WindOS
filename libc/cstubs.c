#include "stddef.h"
#include "stdint.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

int argc;
char **argv;
char **environ;

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

// 0x0b:
int _execve(const char *filename, char *const argv[],
                  char *const envp[]) {
	asm volatile(
		"mov r7, #0x0b\n"
		"ldr r0, %0\n"
		"ldr r1, %1\n"
		"ldr r2, %2\n"
		"svc #0\n"
		:
		: "m" (filename), "m" (argv), "m" (envp)
		:);
	return -1;
}


int execvp(const char *filename, char *const argv[]) {
	char* path = getenv("PATH");
	if (path == NULL || (strchr(filename,'/') != 0) ) { // no PATH or absolute/relative path.
		return execve(filename, argv, environ); // will execute in cwd
	} else { // let's search in path
		char* tok = strtok(path, ":");
		do {
			char* dest_buf = malloc(strlen(filename)+2+strlen(tok));
			strcpy(dest_buf, tok);
			strcat(dest_buf, "/");
			strcat(dest_buf, filename);
			execve(dest_buf, argv, environ);
			free(dest_buf);
		} while ((tok = strtok(NULL, ":")) != NULL);
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
	return res;
}

// TODO: Coder ça
int _isatty(int fd) {
  return 1;
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

    return res;
}
