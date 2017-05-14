#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "../../include/dirent.h"

extern int argc;
extern char** argv;
extern char** environ;

int copy(int fromfd, char* frompath, int destfd, char* destpath, bool recursive) {
	struct stat fs_from;
	struct stat fs_dest;

	int from 	= _openat(fromfd, frompath, O_RDONLY);
	if (from >= 0) {
		fstat(from, &fs_from);
	} else {
		perror("cp");
	}

	int dest	= _openat(destfd, destpath, O_TRUNC | O_WRONLY);
	bool dest_exists = true;
	if (dest < 0) {
		dest_exists = false;
		dest = _openat(destfd, dirname(destpath), O_RDONLY);
		if (dest < 0) {
			perror("cp");
		}
	}
	fstat(dest, &fs_dest);

	if (S_ISREG(fs_from.st_mode) && S_ISREG(fs_dest.st_mode)) {
		char copy_buffer[1024];
		int n;
		while ((n = _read(from, copy_buffer, 1024)) > 0) {
			_write(dest, copy_buffer, n);
		}
	} else if (S_ISREG(fs_from.st_mode)) {
		int new_file;
		if (dest_exists) {
			new_file = _openat(dest, basename(frompath), O_CREAT | O_WRONLY);
		} else {
			new_file = _openat(dest, basename(destpath), O_CREAT | O_WRONLY);
		}

		char copy_buffer[1024];
		int n;
		while ((n = _read(from, copy_buffer, 1024)) > 0) {
			_write(new_file, copy_buffer, n);
		}
		_close(new_file);
	} else if (recursive) {
		if (S_ISREG(fs_dest.st_mode)) {
			errno = ENOTDIR;
			perror("cp");
		} else {
			// target is a dir.
			_mknodat(dest, basename(destpath), S_IFDIR);
			dest = _openat(destfd, destpath, O_RDONLY);

			struct dirent entry;
			while(_getdents(from, &entry) == 0) {
				if (strcmp(entry.d_name, ".") && strcmp(entry.d_name, "..")) {
					copy(from, entry.d_name, dest, entry.d_name, true);
				}
			}
		}
	} else {
		errno = EISDIR;
		perror("cp");
	}

	if (from >= 0) {
		_close(from);
	} else if (dest >= 0) {
		_close(dest);
	}
}

int main() {
	char* params[18];
	for (int i=0;i<argc;i++) {
		params[i] = argv[i];
	}

	bool recursive = false;
	int opt;
	while ((opt = getopt(argc,params,"r")) != -1) {
		switch (opt) {
			case 'r':
				recursive = true;
				break;
			case '?':
				break;
			default:
				fprintf(stderr, "Usage: cp [-r] [path]");
		}
	}

	if ((argv[optind] != NULL) && (argv[optind+1] != NULL)) {
		char* source = argv[optind];
		char* dest = argv[optind+1];
		return copy(AT_FDCWD, source, AT_FDCWD, dest, recursive);
	} else {
		fprintf(stderr, "Usage: cp [-r] [source] [destination]");
		return -1;
	}
}
