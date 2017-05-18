#include "fb.h"
#include "framebuffer.h"
#include <string.h>

bool buffered[MAX_PROCESSES];
int pid_to_window[MAX_PROCESSES];
int n_windows = 0;
int shown_pid = 0;

extern frameBuffer* kernel_framebuffer;



void fb_init() {
	for (int i=0;i<MAX_PROCESSES;i++) {
		pid_to_window[i] = -1;
		buffered[i] = true;
	}
}

int fb_show(int pid) {
	int w = pid_to_window[pid];
	if (w != -1) {
		shown_pid = pid;
		fb_flush(pid);
	}
	return -1;
}

/**
Allocates a window
*/
int fb_open(int pid) {
	int w = pid_to_window[pid];
	if (w == -1 && n_windows != MAX_WINDOWS) {
		pid_to_window[pid] = n_windows;
		n_windows++;
	}
	return -1;
}
int fb_close(int pid) {
	int w = pid_to_window[pid];
	if (w != -1) {
		pid_to_window[pid] = -1;
		for (int i=0;i<MAX_PROCESSES;i++) {
			if (pid_to_window[i] == n_windows-1) {
				pid_to_window[i] = w;
			}
		}
		n_windows--;
		for (int i=0;i<MAX_PROCESSES;i++) {

		}
	}
	return -1;
}

int fb_buffered(int pid, int arg) {
	buffered[pid] = !(arg == 0);
	return 0;
}

bool fb_has_window(int pid) {
	return pid_to_window[pid] != -1;
}

bool fb_has_focus(int pid) {
	return shown_pid == pid;
}

bool fb_is_buffered(int pid) {
	return buffered[pid];
}

int fb_flush(int pid) {
	int w = pid_to_window[pid];
	if (w != -1) {
		memcpy(kernel_framebuffer->bufferPtr, kernel_framebuffer->bufferPtr+(w+1)*kernel_framebuffer->bufferSize, kernel_framebuffer->bufferSize);
		return 0;
	} else {
		return -1;
	}
}

int fb_offset(int pid) {
	int w = pid_to_window[pid];
	return (w+1)*kernel_framebuffer->bufferSize;
}
