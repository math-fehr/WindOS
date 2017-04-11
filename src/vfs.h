#ifndef VFS_H
#define VFS_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "debug.h"

// https://fr.wikipedia.org/wiki/Virtual_File_System
struct superblock_t;
typedef struct superblock_t superblock_t;
typedef struct inode_t inode_t;

typedef struct {
} super_operations_t;

typedef struct vfs_dir_list_t vfs_dir_list_t;

typedef struct {
  vfs_dir_list_t* (*read_dir) (inode_t);
  int (*read) (inode_t, char*, int, int);
  int (*write) (inode_t, char*, int, int);

  int (*mkdir) (inode_t, char*, int);
  int (*rm) (inode_t, char*);
  int (*mkfile) (inode_t, char*, int);
} inode_operations_t;

typedef struct {

} file_operations_t;

#define VFS_MAX_OPEN_FILES 1000
#define VFS_MAX_OPEN_INODES 1000

#define VFS_FIFO         0x1000
#define VFS_CHAR_DEVICE  0x2000
#define VFS_DIRECTORY    0x4000
#define VFS_BLOCK_DEVICE 0x6000
#define VFS_FILE         0x8000
#define VFS_SYMLINK      0xA000
#define VFS_SOCKET       0xC000

#define VFS_ISUID        0x0800
#define VFS_ISGID        0x0400
#define VFS_ISVTX        0x0200

#define VFS_RUSR         0x0100
#define VFS_WUSR         0x0080
#define VFS_XUSR         0x0040

#define VFS_RGRP         0x0020
#define VFS_WGRP         0x0010
#define VFS_XGRP         0x0008

#define VFS_ROTH         0x0004
#define VFS_WOTH         0x0002
#define VFS_XOTH         0x0001

struct inode_t {
  int number;
  int size;
  superblock_t* sb;
  inode_operations_t* op;
  uint32_t attr;
};

typedef struct {
  inode_t* inode;
  int current_offset;
} file_t;

struct superblock_t {
  int id;                  // device ID
  inode_t root;                // root node
  super_operations_t* op;  // superblock methods
};

struct vfs_dir_list_t {
  vfs_dir_list_t* next;
  inode_t inode;
  char* name;
};

typedef struct {
  inode_t inode;
  superblock_t* fs;
} mount_point_t;

typedef struct {
  char str[11];
} perm_str_t;

void vfs_setup();
inode_t vfs_path_to_inode(char *path);
void vfs_mount(superblock_t* sb, char* path);
int vfs_fopen(char* path);
int vfs_fclose(int fd);
int vfs_fseek(int fd, int offset);
int vfs_fwrite(int fd, char* buffer, int length);
int vfs_fread(int fd, char* buffer, int length);
vfs_dir_list_t* vfs_readdir(char* path);
int vfs_attr(char* path);
int vfs_mkdir(char* path, char* name, int permissions);
int vfs_rm(char* path, char* name);
int vfs_mkfile(char* path, char* name, int permissions);
perm_str_t vfs_permission_string(int attr);

#endif
