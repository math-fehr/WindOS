#include "stddef.h"
#include "stdint.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "unistd.h"

// 0x01:
void _exit(int error_code) {
    asm volatile(   "mov r7, #0x01\n"
                    "svc #0");
    while(1){}
}

// 0x2d:
void *_sbrk(intptr_t increment) {
    void* res;
    asm volatile(   "mov r0, %1\n"
                    "mov r7, #0x2d\n"
                    "svc #0\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "r" (increment)
                :);
    return res;
}

// 0x04:
ssize_t _write(int fd, const void* buf, size_t count) {
    ssize_t res;
    asm volatile(   "ldr r0, %1\n"
                    "ldr r1, %2\n"
                    "ldr r2, %3\n"
                    "mov r7, #0x04\n"
                    "svc #0\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "m" (fd), "m" (buf), "m" (count)
                :);
    return res;
}

// 0x06
int _close(int fd) {
    int res;
    asm volatile(   "mov r0, %1\n"
                    "mov r7, #0x06\n"
                    "svc #0\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "r" (fd)
                :);
    return res;
}

// 0x1c
int _fstat(int fd, struct stat* buf) {
    int res;
    asm volatile(   "mov r0, %1\n"
                    "mov r1, %2\n"
                    "mov r7, #0x1c\n"
                    "svc #0\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "r" (fd), "r" (buf)
                :);
    return res;
}

int _isatty(int fd) {
  return 0;
}

// 0x13
off_t _lseek(int fd, off_t offset, int whence) {
    off_t res;
    asm volatile(   "mov r0, %1\n"
                    "mov r1, %2\n"
                    "mov r2, %3\n"
                    "mov r7, #0x13\n"
                    "svc #0\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "r" (fd), "r" (offset), "r" (whence)
                :);
    return res;
}

// 0x03
ssize_t _read(int fd, void* buf, size_t count) {
    ssize_t res;
    asm volatile(   "mov r0, %1\n"
                    "mov r1, %2\n"
                    "mov r2, %3\n"
                    "mov r7, #0x03\n"
                    "svc #0\n"
                    "mov %0, r0"
                :   "=r" (res)
                :   "r" (fd), "r" (buf), "r" (count)
                :);
    return res;
}
