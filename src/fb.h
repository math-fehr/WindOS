
#include <stdbool.h>

#include "kernel.h"

#define FB_WIDTH 	0
#define FB_HEIGHT 	1
#define FB_OPEN 	2
#define FB_CLOSE 	3
#define FB_SHOW 	4
#define FB_FLUSH 	5
#define FB_BUFFERED 6

#define MAX_WINDOWS 8

int fb_show(int pid);
int fb_open(int pid);
int fb_close(int pid);
int fb_count();
int fb_flush(int pid);

bool fb_has_focus(int pid);
int fb_buffered(int pid, int arg);
