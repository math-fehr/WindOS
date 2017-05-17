#include "vfs.h"


#define PIPE_BUFFER_BLOCK_SIZE 1024
#define MAX_PIPES 128

typedef struct pipe_block pipe_block;
struct pipe_block {
	char buffer[PIPE_BUFFER_BLOCK_SIZE];
	pipe_block* next;
};

inode_t mkpipe();
bool free_pipe(int index);
int pipe_read(inode_t pipe, char* buffer, int count, int ofs);
int pipe_write(inode_t pipe, char* buffer, int count, int ofs);
