#include "../../include/syscalls.h"
#include "../../include/signals.h"
#include <stdio.h>
#include <signal.h>


siginfo_t caught_signal;

void handler(int signal) {
	printf("Caught signal %d from pid %d\n", signal, caught_signal.si_pid);
	sigreturn();
}

int main() {
	sigaction(SIGSEGV, handler, &caught_signal);
	sigaction(SIGKILL, handler, &caught_signal);
	sigaction(SIGTERM, handler, &caught_signal);

	while(1) {}
}
