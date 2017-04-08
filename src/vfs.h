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
  vfs_dir_list_t* (*read_dir) (superblock_t*,inode_t);
} inode_operations_t;

typedef struct {

} file_operations_t;

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
  superblock_t* sb;
  inode_operations_t* op;
  uint32_t attr;
};

typedef struct {
  superblock_t* sb;
  inode_t* inode;
  file_operations_t* op;
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
int vfs_fopen(char* path, int mode);
int vfs_fclose(int fd);
int vfs_fseek(int fd, int offset);
int vfs_fwrite(int fd, char* buffer, int length);
int vfs_fread(int fd, char* buffer, int length);
vfs_dir_list_t* vfs_readdir(char* path);
int vfs_permissions(char* path);
void vfs_mkdir(char* path, char* name, int permissions);
void vfs_rmdir(char* path, char* name);
void vfs_mkfile(char* path, char* name, int permissions);
void vfs_rmfile(char* path, char* name);
perm_str_t vfs_permission_string(int attr);

#endif
