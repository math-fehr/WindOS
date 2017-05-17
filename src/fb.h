
#include <stdbool.h>

#define FB_WIDTH 0
#define FB_HEIGHT 1
#define FB_OPEN 2
#define FB_CLOSE 3
#define FB_SHOW 4

int fb_show(int pid);
int fb_open(int pid);
int fb_close(int pid);
int fb_count();

bool fb_has_focus(int pid);
