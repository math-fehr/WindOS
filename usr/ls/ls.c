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
	int r, c;
	r=40;
	c=80;
	//term_get_size(&r, &c);

	int fd;

	char* target = ".";
	int opt;

	bool line = false;
	bool inode = false;
	bool all = false;

	optind = 1;
	opterr = 0;

	char* params[18];
	for (int i=0;i<argc;i++) {
		params[i] = argv[i];
	}

	while ((opt = getopt(argc,params,"ali")) != -1) {
		switch (opt) {
			case 'l':
				line = true;
				break;
			case 'i':
				inode = true;
				break;
			case 'a':
				all = true;
				break;
			case '?':
				break;
			default:
				fprintf(stderr, "Usage: ls [-lia] [directory]");
		}
	}

	if (argv[optind] != NULL) {
		target = argv[optind];
	}

	fd = _open(target,O_RDONLY);

	if (fd < 0) {
		perror("ls");
		return 1;
	} else {

		struct dirent* entry = malloc(sizeof(struct dirent));
		int max_size 	= 0;
		int cur_size    = 0;

		int max_inode 	= 0;
		int cur_inode 	= 0;

		int max_fsz 	= 0;

		struct stat fs;

		int total = 0;

		while(_getdents(fd,entry) == 0) {
			if (all || entry->d_name[0] != '.') {
				total++;
				cur_size = strlen(entry->d_name);
				cur_inode = entry->d_ino;

				if (cur_size > max_size) {
					max_size = cur_size;
				}

				if (inode) {
					if (cur_inode > max_inode) {
						max_inode = cur_inode;
					}
				}

				if (line) {
					int fd_child = _openat(fd, entry->d_name, O_RDONLY);
					fstat(fd_child, &fs);
					_close(fd_child);

					if (fs.st_size > max_fsz) {
						max_fsz = fs.st_size;
					}
				}
			}
		};

		_lseek(fd,0,SEEK_SET);

		int i_inode=1;
		int i_fsz=1;
		int val=1;

		do {
			if (val <= max_inode) {
				i_inode++;
			}
			if (val <= max_fsz) {
				i_fsz++;
			}
			val *= 10;
		} while (val <= max_inode || val <= max_fsz);

		char fmt[128];
		char fmt_dir[128];

		int pos = 0;
		int fmt_size = 0;

		if (inode) {
			pos = sprintf(fmt, "%% %dd ", i_inode);
			fmt_size += i_inode + 1;
		}

		if (line) {
			pos += sprintf(fmt+pos, "%%s %% %dd ", i_fsz);
			fmt_size += i_fsz + 10 + 2;
		}
		fmt_size += max_size + 1;

		strcpy(fmt_dir, fmt);


		sprintf(fmt_dir+pos, "\033[1m\033[38;5;12m%% %ds \033[0m", max_size);
		sprintf(fmt+pos, "%% %ds ", max_size);


		int n_per_lines = c/fmt_size;
		int k = 0;

		while(_getdents(fd,entry) == 0) {
			if (all || (entry->d_name[0] != '.')) {
				if (line) {
					int fd_child = _openat(fd, entry->d_name, O_RDONLY);
					fstat(fd_child, &fs);
					_close(fd_child);
				}

				char modestr[11];
				get_permission_string(fs.st_mode, modestr);
				char* fmt_f;
				if (S_ISDIR(entry->d_type)) {
					fmt_f = fmt_dir;
				} else {
					fmt_f = fmt;
				}
				if (line & inode) {
					printf(fmt_f, entry->d_ino, modestr, fs.st_size, entry->d_name);
				} else if (line) {
					printf(fmt_f, modestr, fs.st_size, entry->d_name);
				} else if (inode) {
					printf(fmt_f, entry->d_ino, entry->d_name);
				} else {
					printf(fmt_f, entry->d_name);
				}
				k++;
				if (line || k == n_per_lines) {
					k = 0;
					printf("\n");
				}
			}
		}
		if (k != 0) {
			printf("\n");
		}
		return 0;
	}

}
