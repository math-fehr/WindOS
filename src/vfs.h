#ifndef VFS_H
#define VFS_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "debug.h"

#include "sys/stat.h"

// https://fr.wikipedia.org/wiki/Virtual_File_System
struct superblock_t;
typedef struct superblock_t superblock_t;
typedef struct inode_t inode_t;

typedef struct {
} super_operations_t;

typedef struct vfs_dir_list_t vfs_dir_list_t;

typedef struct {
  vfs_dir_list_t* (*read_dir) (inode_t*);
  int (*read) (inode_t*, char*, int, int);
  int (*write) (inode_t*, char*, int, int);

  int (*mkdir) (inode_t*, char*, int);
  int (*rm) (inode_t*, char*);
  int (*mkfile) (inode_t*, char*, int);
} inode_operations_t;

typedef struct {

} file_operations_t;

#define VFS_MAX_OPEN_FILES 1000
#define VFS_MAX_OPEN_INODES 1000


struct inode_t {
  superblock_t* sb;
  inode_operations_t* op;
  struct stat st;
};

typedef struct {
  inode_t* inode;
  int current_offset;
} file_t;

struct superblock_t {
  int id;                  // device ID
  inode_t* root;           // root node
  super_operations_t* op;  // superblock methods
};

struct vfs_dir_list_t {
  vfs_dir_list_t* next;
  inode_t* inode;
  char* name;
};

typedef struct {
  inode_t* inode;
  superblock_t* fs;
} mount_point_t;

typedef struct {
  char str[11];
} perm_str_t;

void vfs_setup();
inode_t* vfs_path_to_inode(char *path);
void vfs_mount(superblock_t* sb, char* path);

int vfs_fwrite(inode_t* fd, char* buffer, int size, int position);
int vfs_fread(inode_t* fd, char* buffer, int size, int position);
vfs_dir_list_t* vfs_readdir(char* path);
int vfs_attr(char* path);
int vfs_mkdir(char* path, char* name, int permissions);
int vfs_rm(char* path, char* name);
int vfs_mkfile(char* path, char* name, int permissions);
perm_str_t vfs_permission_string(int attr);

#endif
