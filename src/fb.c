#include "fb.h"
#include "framebuffer.h"
#include <string.h>


/** \file fb.c
 *  \brief Framebuffer handler.
 *
 *  Here are declared the framebuffer management features, that are called by the
 *  dev pseudo-filesystem.
 */

/** \var bool buffered[MAX_PROCESSES]
 *  \brief A bit table to save if the current process wants to be buffered, or
 *  to be directly displayed on the framebuffer.
 */
bool buffered[MAX_PROCESSES];

/** \var int pid_to_window[MAX_PROCESSES]
 *  \brief Maps process id to displayed window.
 */
int pid_to_window[MAX_PROCESSES];

int n_windows = 0;
int shown_pid = 0;

/** \var int win_list[MAX_WINDOWS]
 *  \brief Each window points to its ancestor, the oldest window pointing to -1.
 */
int win_list[MAX_WINDOWS];
int top_win;

extern frameBuffer* kernel_framebuffer;

/** \fn void fb_init()
 *  \brief Initialize data structures of the framebuffer system.
 */
void fb_init() {
	for (int i=0;i<MAX_PROCESSES;i++) {
		pid_to_window[i] = -1;
		buffered[i] = true;
	}
	for (int i=0;i<MAX_WINDOWS;i++) {
		win_list[i] = -1;
	}
	top_win = -1;
}

/** \fn int fb_win_to_pid(int w)
 *  \brief Reverse the pid_to_window table.
 *  \param w The looked up window.
 *  \return The pid associated to this window if found. Else -1.
 */
int fb_win_to_pid(int w) {
	for (int i=0;i<MAX_PROCESSES;i++) {
		if (pid_to_window[i] == w) {
			return i;
		}
	}
	return -1;
}

/** \fn int fb_show(int pid)
 *  \brief Bring a process window to front.
 *  \param pid The process to show.
 *  \return -1
 */
int fb_show(int pid) {
	int w = pid_to_window[pid];
	if (w != -1) {
		shown_pid = pid;
		fb_flush(pid);

		int child = -1;
		for (int i=0;i<MAX_WINDOWS;i++) {
			if (win_list[i] == w) {
				child = i;
			}
		}
		if (win_list[w] != -1 && child != -1) {
			win_list[child] = win_list[w];
		}
		win_list[w] = top_win;
		top_win = w;
	}
	return -1;
}

/** \fn int fb_open(int pid)
 *  \brief Allocates a window for a process.
 *  \param pid Process identifier.
 *  \return The number of the window on success. -1 on failure. Failure can be
 *  because the process has already opened a window, or because there are no
 *  available window.
 */
int fb_open(int pid) {
	int w = pid_to_window[pid];
	if (w == -1 && n_windows != MAX_WINDOWS) {
		pid_to_window[pid] = n_windows;
		n_windows++;
	}
	return -1;
}

/** \fn int fb_close(int pid)
 *  \brief Close a process' window.
 *  \param pid The process identifier.
 *  \return -1.
 *
 *  When closing a window, brings the ancestor window to the front.
 */
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

		if (win_list[w] != -1) {
			top_win = win_list[w];
			win_list[w] = -1;
			shown_pid = fb_win_to_pid(top_win);
			fb_flush(shown_pid);
		}
	}
	return -1;
}

/** \fn int fb_buffered(int pid, int arg)
 *  \brief Updates the buffering mode of a process.
 *  \param pid The process to update.
 *  \param arg If arg == 0, unbuffered. If arg != 0, buffered mode.
 */
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

/** \fn int fb_flush(int pid)
 *  \brief Flush a process' buffer to the displayed framebuffer.
 */
int fb_flush(int pid) {
	int w = pid_to_window[pid];
	if (w != -1) {
		memcpy(kernel_framebuffer->bufferPtr, kernel_framebuffer->bufferPtr+(w+1)*kernel_framebuffer->bufferSize, kernel_framebuffer->bufferSize);
		return 0;
	} else {
		return -1;
	}
}

/** \fn int fb_offset(int pid)
 *  \brief Get the offset locating a process' buffer.
 *
 *  \warning The process should have an allocated window. 
 */
int fb_offset(int pid) {
	int w = pid_to_window[pid];
	return (w+1)*kernel_framebuffer->bufferSize;
}
