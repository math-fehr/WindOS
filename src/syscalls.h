#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "kernel.h"
#include "interrupts.h"
#include "timer.h"
#include "serial.h"
#include "debug.h"
#include "scheduler.h"
#include "mmu.h"
#include "malloc.h" 

uint32_t svc_exit();
uint32_t svc_sbrk(uint32_t ofs);
uint32_t svc_fork();
uint32_t svc_write(uint32_t fd, char* buf, size_t cnt);
uint32_t svc_close(uint32_t fd);
uint32_t svc_fstat(uint32_t fd, struct stat* dest);
uint32_t svc_read(uint32_t fd, char* buf, size_t cnt);
uint32_t svc_time(time_t* tloc);
uint32_t svc_execve(char* path, const char** argv, const char** env);
pid_t 	 svc_waitpid(pid_t pid, int* wstatus, int options);
char* 	 svc_getcwd(char* buf, size_t cnt);
uint32_t svc_chdir(char* path);

#endif
