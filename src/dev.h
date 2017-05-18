#include <stdlib.h>
#include <errno.h>
#include "vfs.h"
#include "serial2.h"


#define DEV_ROOT    2
#define DEV_NULL    3
#define DEV_RANDOM  4
#define DEV_SERIAL  5
#define DEV_SERIAL2 8
#define DEV_ZERO    6
#define DEV_FB 		7

superblock_t* dev_initialize(int id);
vfs_dir_list_t* dev_append_elem(inode_t inode, char* name, vfs_dir_list_t* lst);
vfs_dir_list_t* dev_lsdir(inode_t from) ;
int dev_fread(inode_t from, char* buf, int size, int pos);
int dev_fwrite(inode_t from, char* buf, int size, int pos);

int dev_ioctl(inode_t from, int cmd, int arg);
