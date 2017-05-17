#include "pipefs.h"
#include <stdlib.h>

pipe_block*		pipe_buffers[MAX_PIPES];
bool 			pipe_map[MAX_PIPES];
int 			buffer_begin[MAX_PIPES];
int 			buffer_end[MAX_PIPES];

static inode_operations_t pipe_operations = {
	.read = pipe_read,
	.write = pipe_write,
};

inode_t mkpipe() {
	inode_t result;
	result.ref_count = 1;

	int i=0;
	for(;!pipe_map[i];i++) {}
	pipe_map[i] = true;
	pipe_buffers[i] = malloc(sizeof(pipe_block));
	pipe_buffers[i]->next = NULL;
	result.st.st_ino = i;
	result.st.st_mode = S_IFIFO;
	result.st.st_dev = -1;
	result.op = &pipe_operations;
	return result;
}

bool free_pipe(int index) {
	if (!pipe_map[index]) {
		return false;
	}

	pipe_block* lst = pipe_buffers[MAX_PIPES];
	pipe_block* prev = NULL;
	while (lst != NULL) {
		prev = lst;
		lst = lst->next;
		free(prev);
	}
	pipe_map[index] = false;
	buffer_begin[index] = 0;
	buffer_end[index] = 0;
	return true;
}

// TODO: Checks
int pipe_read(inode_t pipe, char* buffer, int count, int ofs) {
	(void) ofs; // No seek on a pipe.
	int i = pipe.st.st_ino;
	pipe_block* pos = pipe_buffers[i];
	// pos pointss to the first block.

	int to_read = count;
	int pos_blk = buffer_begin[i];

	int size_read = 0;

	//kernel_printf("read %d %d %d\n", count, buffer_begin[i], buffer_end[i]);

	while (to_read > 0 && (pos->next != NULL || pos_blk != buffer_end[i])) {
		int size;
		if (pos->next == NULL) {
			size = min(to_read, buffer_end[i] - pos_blk);
		} else {
			size = min(to_read, PIPE_BUFFER_BLOCK_SIZE - pos_blk);
		}

		memcpy(buffer, pos->buffer+pos_blk, size);

		buffer += size;
		to_read -= size;
		pos_blk += size;
		size_read += size;

		// Finished to read the block, free the block.
		if (pos_blk == PIPE_BUFFER_BLOCK_SIZE) {
			pos_blk = 0;
			pipe_block* next = pos->next;
			free(pos);
			pos = next;
		}
	}

	buffer_begin[i] = pos_blk;
	pipe_buffers[i] = pos;
//	kernel_printf("read %d %d %d\n", count, buffer_begin[i], buffer_end[i]);
	return size_read;
}



int pipe_write(inode_t pipe, char* buffer, int count, int ofs) {

	(void) ofs; // No seek on a pipe.
	int i = pipe.st.st_ino;
	pipe_block* pos = pipe_buffers[i];
	while (pos->next != NULL) {
		pos = pos->next;
	}
	//kernel_printf("write %d %d %d\n", count, buffer_begin[i], buffer_end[i]);

	// pos points to the last block.
	int to_write = count;
	int pos_blk = buffer_end[i];

	while (to_write > 0) {
		int size = min(to_write, PIPE_BUFFER_BLOCK_SIZE - pos_blk);
		memcpy(pos->buffer+pos_blk, buffer, size);
		to_write -= size;
		pos_blk += size;
		buffer += size;

		// Filled the block, create a new block.
		if (pos_blk == PIPE_BUFFER_BLOCK_SIZE) {
			pos_blk = 0;
			pipe_block* new_block = malloc(sizeof(pipe_block));
			new_block->next = NULL;
			pos->next = new_block;
			pos = new_block;
		}
	}

	buffer_end[i] = pos_blk;
	kernel_printf("write %d %d %d\n", count, buffer_begin[i], buffer_end[i]);
	return to_write;
}
