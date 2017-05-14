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


/*
 * These structures are the interface between the virtual file system and
 * real filesystems such as EXT2.
 *
 * On fail, returns -1 and set the appropriate errno.
 */
typedef struct {
  vfs_dir_list_t* (*read_dir) (inode_t);
  int (*read) (inode_t, char*, int, int);
  int (*write) (inode_t, char*, int, int);
  int (*mkdir) (inode_t, char*, int);
  int (*rm) (inode_t, char*);
  int (*mkfile) (inode_t, char*, int);
  int (*ioctl) (inode_t, int, int);
  int (*resize) (inode_t, int);
} inode_operations_t;


// Represents an inode.
struct inode_t {
  superblock_t* sb;
  inode_operations_t* op;
  struct stat st;
};


struct superblock_t {
  int id;                  // device ID
  inode_t root;           // root node
  super_operations_t* op;  // superblock methods
};


/*
 * Structures and methods that the kernel will use to work on the VFS.
 */

// Represents an open file descriptor.
typedef struct {
  inode_t* inode;
  int current_offset;
} file_t;

// Represents a directory listing.
struct vfs_dir_list_t {
  vfs_dir_list_t* next;
  inode_t inode;
  char* name;
};

// Fixed length permission string
typedef struct {
  char str[11];
} perm_str_t;

// Internal structure representing mount points.
typedef struct {
  inode_t inode;
  superblock_t* fs;
} mount_point_t;

/*
 * VFS functions.
 */
void 		vfs_setup();
inode_t 	vfs_path_to_inode(inode_t* root, char *path);
inode_t 	vfs_mknod(inode_t base, char* name, mode_t mode, dev_t dev);
char* 		vfs_inode_to_path(inode_t inode, char* buf, size_t cnt);
void 		vfs_mount(superblock_t* sb, char* path);
int 		vfs_fwrite	(inode_t fd, char* buffer, int size, int position);
int 		vfs_fread	(inode_t fd, char* buffer, int size, int position);
vfs_dir_list_t* vfs_readdir(char* path);
int 		vfs_attr	(char* path);
int 		vfs_mkdir	(char* path, char* name, int permissions);
int 		vfs_rm		(char* path, char* name);
int 		vfs_mkfile	(char* path, char* name, int permissions);
perm_str_t 	vfs_permission_string(int attr);
void 		free_vfs_dir_list(vfs_dir_list_t*);
#endif
