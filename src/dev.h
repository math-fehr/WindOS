#include "vfs.h"

#define N_MAX_DEVICES 100

#define DEV_ROOT    2
#define DEV_NULL    3
#define DEV_RANDOM  4
#define DEV_SERIAL  5
#define DEV_ZERO    6
 


superblock_t* dev_initialize(int id);
vfs_dir_list_t* dev_append_elem(inode_t* inode, char* name, vfs_dir_list_t* lst);
vfs_dir_list_t* dev_lsdir(inode_t* from) ;
int dev_fread(inode_t* from, char* buf, int size, int pos);
int dev_fwrite(inode_t* from, char* buf, int size, int pos);
