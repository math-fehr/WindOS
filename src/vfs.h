#ifndef VFS_H
#define VFS_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "debug.h"

#include "sys/stat.h"

/** \def MAX_MNT
 *	\brief Number of mount points limit.
 */
#define MAX_MNT 20

/** \def MAX_ID
 * 	\brief Device ID upper bound.
 */
#define MAX_ID 1024

struct superblock_t;
typedef struct superblock_t superblock_t;
typedef struct inode_t inode_t;


typedef struct vfs_dir_list_t vfs_dir_list_t;


/** \struct inode_operations_t
 * 	\brief Available operations on a given inode.
 *
 * 	These structures are the interface between the virtual file system and
 * 	real filesystems such as EXT2.
 *
 * 	On fail, returns -1 and set the appropriate errno.
 */
typedef struct {
  vfs_dir_list_t* (*read_dir) (inode_t); ///< Dir: Read a directory.
  int (*read) (inode_t, char*, int, int); ///< File: Read a file.
  int (*write) (inode_t, char*, int, int); ///< File: Write a file.
  int (*mkdir) (inode_t, char*, int); ///< Dir: Create a directory.
  int (*rm) (inode_t, char*); ///< Dir: Remove entry.
  int (*mkfile) (inode_t, char*, int); ///< Dir: Create a file.
  int (*ioctl) (inode_t, int, int); ///< File: send control commands to device.
  int (*resize) (inode_t, int); ///< File: resize file content.
} inode_operations_t;

/**	\struct inode_t
 *	\brief Inode interface.
 */
struct inode_t {
  superblock_t* sb; ///< Inode's superblock.
  inode_operations_t* op; ///<
  struct stat st;
};


struct superblock_t {
  int id;                  	// device ID
  inode_t root;           	// root node
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
