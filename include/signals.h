#ifndef USR_SIGNALS_H
#define USR_SIGNALS_H


/// Must be coherent with process.h
typedef struct {
	int si_signo;
	int si_pid;
} siginfo_t;


#endif
