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
  superblock_t* sb; ///< Pointer to superblock.
  inode_operations_t* op; ///< Pointer to inode's operations.
  struct stat st; ///< Inode descriptor data.
};

/** \struct superblock_t
 *	\brief Filesystem interface.
 */
struct superblock_t {
  int id;                  	///< Device ID
  inode_t root;           	///< Root node
};


/*
 * Structures and methods that the kernel will use to work on the VFS.
 */

/** \struct vfs_dir_list_t
 * 	\brief A directory listing.
 */
struct vfs_dir_list_t {
  vfs_dir_list_t* next; ///< Pointer to next entry.
  inode_t inode; 		///< Current entry's inode.
  char* name;			///< Current entry's name.
};

/** \struct mount_point_t
 *	\brief Mount point structure.
 */
typedef struct {
  inode_t inode; ///< Inode on which the FS is mounted.
  superblock_t* fs; ///< Mounted filesystem.
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
void 		free_vfs_dir_list(vfs_dir_list_t*);
#endif
