#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "kernel.h"
#include "interrupts.h"
#include "timer.h"
#include "serial.h"
#include "debug.h"
#include "scheduler.h"
#include "mmu.h"
#include "malloc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <signal.h>

#include "../include/dirent.h"
#include "../include/signals.h"

bool 	 his_own(process* p, void* pointer);

uint32_t svc_exit(int code);
uint32_t svc_sbrk(uint32_t ofs);
uint32_t svc_fork();
uint32_t svc_time(time_t* tloc);
uint32_t svc_execve(char* path, const char** argv, const char** env);
pid_t 	 svc_waitpid(pid_t pid, int* wstatus, int options);
char* 	 svc_getcwd(char* buf, size_t cnt);
uint32_t svc_chdir(char* path);


int svc_kill(pid_t pid, int sig);
int svc_sigaction(int signum, void (*handler)(int), siginfo_t* siginfo);
void svc_sigreturn();
#endif
