#include "stdio.h"
#include "stdlib.h"
#include "../../include/dirent.h"
#include "../../include/termfeatures.h"
#include "../../include/syscalls.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

extern int argc;
extern char** argv;

int main() {
	int fd = _open("/proc/", O_RDONLY);
	if (fd >= 0) {
		struct dirent entry;
		int result;
		printf("%-32s %4s %5s %5s\n", "Name", "State", "PID", "PPID");
		while((result = _getdents(fd, &entry)) == 0) {
			if (entry.d_name[0] != '.') {
				int proc_fd = _openat(fd, entry.d_name, O_RDONLY);
				char buffer[256];
				_read(proc_fd, buffer, 256);
				char* token = strtok(buffer, "\n");
				int cnt = 0;
				while (token != NULL) {
					switch (cnt) {
						case 0: // Name:
							printf("%32s ", token+5);
							break;
						case 1: // State:
							printf("%2s", token+6);
							break;
						case 2:  // PID:
							printf(" %3s", token+4);
							break;
						case 3: // PPID:
							printf("  %3s", token+5);
							break;
					}

					cnt++;
					token = strtok(NULL, "\n");
				}
				_close(proc_fd);
				printf("\n");
			}
		}
		_close(fd);
	} else {
		perror("ps");
	}
}
