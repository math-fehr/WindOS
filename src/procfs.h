#include "process.h"
#include "scheduler.h"
#include "dev.h"
#include <stdio.h>
#include <errno.h>

superblock_t* proc_initialize(int id);
vfs_dir_list_t* proc_lsdir(inode_t from);
int proc_fread(inode_t from, char* buf, int size, int pos);
