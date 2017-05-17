/** \file vfs.c
 *	\brief Virtual File System feature.
 *
 *	This code provides an abstraction of the notion of filesystem.
 *	It is the interface between filesystem drivers and the rest of the kernel.
 *
 *  Inspiration: https://fr.wikipedia.org/wiki/Virtual_File_System
 */

#include "vfs.h"
#include <stdlib.h>
#include <errno.h>

/**	\var inode_t root_inode
 *	\brief The base inode, representng /.
 */
inode_t root_inode;
/** \var mount_point_t mount_points[MAX_MNT]
 * 	\brief Filesystem mount points.
 */
mount_point_t mount_points[MAX_MNT];
/** \var int dev_to_mnt[MAX_ID]
 *	\brief Helper table to translate a device id to it's index in the mount table.
 */
int dev_to_mnt[MAX_ID];

/** \var int nmounted
 *	The number of mounted filesystems.
 */
int nmounted = 0;


void vfs_setup() {}

/** \fn void vfs_mount(superblock_t* sb, char* path)
 * 	\brief Mounts a filesystem into the VFS.
 *	\param sb Superblock representing the FS.
 *	\param path Directory in which the FS should be mounted.
 *
 *	\warning No checks are made, path should refer to an existing directory.
 */
void vfs_mount(superblock_t* sb, char* path) {
  if (nmounted == 0) {
    kernel_printf("[INFO][VFS] FS mounted.\n");
    root_inode = sb->root;
    nmounted++;
	dev_to_mnt[sb->id] = 0;
  } else {
      inode_t r = vfs_path_to_inode(NULL,path);
      if (errno > 0) {
		  kdebug(D_VFS,5,"vfs: %s\n",strerror(errno));
      } else {
          mount_points[nmounted].inode = r;
          mount_points[nmounted].fs = sb;
		  dev_to_mnt[sb->id] = nmounted;
          nmounted++;
      }
  }
}

/** \fn void free_vfs_dir_list(vfs_dir_list_t* lst)
 *  \param Free a malloc'd vfs_dir_list.
 */
void free_vfs_dir_list(vfs_dir_list_t* lst) {
	vfs_dir_list_t* prec;
	while (lst != NULL) {
		prec = lst;
		lst = lst->next;
		free(prec->name);
		free(prec);
	}
}

/** \fn inode_t vfs_parent(inode_t base, int* ancestor, char* writebuf)
 * 	\brief Returns the parent of an inode.
 *	\param base The base inode.
 *	\param ancestor A pointer to the base inode's children which we want to retrieve the name.
 *	\param writebuf If ancestor is not NULL, and found during the exploration, writes its name to writebuf.
 *	\return Parent inode of base.
 *
 *	This function is a helper function for vfs_inode_to_path. It finds the parent
 *	of an inode, and by the way finds the name of one of it's children.
 */
inode_t vfs_parent(inode_t base, int* ancestor, char* writebuf) {
	inode_t mnt_root = mount_points[dev_to_mnt[base.sb->id]].fs->root;
	if (base.st.st_ino == mnt_root.st.st_ino) {
		// If this is the root of a mount point, teleport back.
		base = mount_points[dev_to_mnt[base.sb->id]].inode;
	}

	if (base.op->read_dir == NULL) {
		return base;
	}
	vfs_dir_list_t* result = base.op->read_dir(base);
	vfs_dir_list_t* result_start = result;

	if (ancestor != NULL) {
		if (*ancestor == -1)
			writebuf[0] = 0;
	}

	inode_t res;
	while(result) {
		if (strcmp(result->name, "..") == 0) {
			res = result->inode;
		}

		if (ancestor != NULL && result->inode.st.st_ino == *ancestor) {
			strcpy(writebuf, result->name);
		}
		result = result->next;
	}
	if (ancestor != NULL) {
		*ancestor = base.st.st_ino;
	}
	free_vfs_dir_list(result_start);
	return res;
}

/** \fn char* vfs_inode_to_path(inode_t base, char* buf, size_t size)
 * 	\brief Finds the path, given an inode.
 *	\param base The starting inode.
 *	\param buf Buffer where we write the path.
 *	\param size Size of this buffer.
 *	\return buf
 *
 *	\bug Size is not used, buffer overflow can occur.
 */
char* vfs_inode_to_path(inode_t base, char* buf, size_t size) {
	int i=size-2;
	buf[i+1] = 0;
	buf[i] = '/';
	// i points to the beginning of the buffer.
	int ancestor = -1;

	char tmp[255];
	while (base.st.st_ino != root_inode.st.st_ino
		|| base.st.st_dev != root_inode.st.st_dev)
	{
		base = vfs_parent(base, &ancestor, tmp);
		if (strlen(tmp) > 0) {
			i -= strlen(tmp);
			memcpy(&buf[i], tmp, strlen(tmp));
			buf[i-1] = '/';
			i--;
		}
	}

	base = vfs_parent(base, &ancestor, tmp);
	if (strlen(tmp) > 0) {
		i -= strlen(tmp);
		memcpy(&buf[i], tmp, strlen(tmp));
		buf[i-1] = '/';
		i--;
	}

	for (int j=i;(unsigned)j<size;j++) {
		buf[j-i] = buf[j];
	}

	return buf;
}

/** \fn inode_t vfs_path_to_inode(inode_t* base, char *path_)
 * 	\brief Given a path, gives the inode.
 *	\param base If the path is relative, it is relative to base.
 * 	\param path_ Access path.
 * 	\return The inode refered by this path (can be relative to base).
 *
 *	\warning If it fails, sets the errno accordingly.
 */
inode_t vfs_path_to_inode(inode_t* base, char *path_) {
	errno = 0;
  	char* path = malloc(strlen(path_)+1);
  	strcpy(path, path_);

	kdebug(D_VFS, 1, "Resolving path %s\n", path);
	inode_t position = root_inode;

	if (path[0] != '/' && base == NULL) {
		free(path);
		kdebug(D_VFS, 5, "Relative path but no base inode.\n");
		errno = ENOENT;
		return position;
	}

	if (path[0] != '/') // working on a relative path.
		position = *base;

	char* token = strtok(path, "/");
	if (token == NULL) {
		return position;
	}

	do {
		if (!S_ISDIR(position.st.st_mode)) {
			kdebug(D_VFS, 4, "Bad path name: %s failed on token %s\n", path_, token);
			free(path);
			errno = ENOENT;
			return position;
		}

		if (strcmp(token, "..") == 0) {
			position = vfs_parent(position, NULL, NULL);
		} else {
			if (position.op->read_dir == NULL) {
				errno = -1;
				return position;
			}
			vfs_dir_list_t* result = position.op->read_dir(position);
			vfs_dir_list_t* result_start = result;
			bool found = false;
			while (result != 0 && !found) {
				if (strcmp(result->name, token) == 0) {
					position = result->inode;
					kdebug(D_VFS, 2, "%s:%d\n",token,result->inode.st.st_ino);
					found = true;
					for (int i=1;i<nmounted;i++) {
		                kdebug(D_VFS, 2, "Mount transition: %s %d %d\n",
											token,
											mount_points[i].inode.st.st_ino,
											result->inode.st.st_ino);
						if (result->inode.st.st_ino == mount_points[i].inode.st.st_ino
						 && result->inode.sb->id == mount_points[i].inode.sb->id) {
		                    position = mount_points[i].fs->root;
		                }
					}
				}

				result = result->next;
			}
			free_vfs_dir_list(result_start);

			if (!found) {
				kdebug(D_VFS, 4, "Bad path name: %s failed on token %s\n", path_, token);
				free(path);
				errno = ENOENT;
				return position;
			}
		}
	} while ((token = strtok(NULL, "/")) != NULL);

	kdebug(D_VFS, 2, "path_to_ino: %s -> %d\n", path_, position.st.st_ino);
	free(path);

	return position;
}

/** \fn inode_t vfs_mknod(inode_t base, char* name, mode_t mode, dev_t dev)
 *	\brief Creates a node in the filesystem.
 *	\param base Base inode.
 *	\param name Path to create.
 *	\param mode Mode of the created inode.
 *	\param dev Device mode for device inodes.
 *	\return The newly created inode.
 *
 *	\warning On fail, sets errno accordingly.
 */
inode_t vfs_mknod(inode_t base, char* name, mode_t mode, dev_t dev) {
	(void) dev;
	inode_t d;
	errno = 0;
	if (strlen(name) == 0 || (strchr(name, '/') != NULL)) {
		errno = -EINVAL;
		return d;
	}

	if (S_ISDIR(mode)) {
		if (base.op->mkdir != NULL) {
			base.op->mkdir(base, name, mode);
		}
		return vfs_path_to_inode(&base, name);
	} else {
		if (base.op->mkfile != NULL) {
			base.op->mkfile(base, name, mode);
		}
		return vfs_path_to_inode(&base, name);
	}
}

/** \fn int vfs_fwrite(inode_t fd, char* buffer, int length, int offset)
 *	\brief Writes to a file.
 *	\param fd File inode
 *	\param buffer Buffer to write.
 *	\param length Number of bytes to write.
 *	\param offset Position in the file to write.
 *	\return Number of bytes written on success. -1 on error with errno set.
 */
int vfs_fwrite(inode_t fd, char* buffer, int length, int offset) {
    if (S_ISDIR(fd.st.st_mode)) {
		errno = EISDIR;
        return -1;
    }

    if (offset > fd.st.st_size) {
        offset = fd.st.st_size;
    }
	if (fd.op->write == NULL) {
		errno = -1;
		return -1;
	}
    return fd.op->write(fd, buffer, length, offset);
}

/** \fn int vfs_fread(inode_t fd, char* buffer, int length, int offset)
 * 	\brief Read a file.
 *	\param fd File inode.
 *	\param buffer Destination buffer.
 *  \param length Number of bytes to read.
 * 	\param offset Position in the file to read.
 *	\return Number of bytes read on success. -1 on error with errno set.
 */
int vfs_fread(inode_t fd, char* buffer, int length, int offset) {
	if (S_ISDIR(fd.st.st_mode)) {
	  	errno = EISDIR;
	  	return -1;
	}


	if (offset > fd.st.st_size) {
	  offset = fd.st.st_size;
	}
	if (fd.op->read == NULL) {
		errno = -1;
		return -1;
	}
	return fd.op->read(fd, buffer, length, offset);
}

/** \fn vfs_dir_list_t* vfs_readdir(char* path)
 * 	\brief Read a directory.
 *	\param path Absolute position of the directory.
 *	\return A directory entry list on success. NULL on error, with errno set.
 */
vfs_dir_list_t* vfs_readdir(char* path) {
	inode_t position = vfs_path_to_inode(NULL, path);
	if (errno == 0) {
		if (S_ISDIR(position.st.st_mode)) {
			if (position.op->read_dir == NULL) {
		    	errno = -1;
		    	return NULL;
		    }
			vfs_dir_list_t* r = position.op->read_dir(position);
		    return r;
		} else {
			errno = ENOTDIR;
			return NULL;
		}
	} else {
	    return NULL;
	}
}

/** \fn int vfs_attr(char* path)
 * 	\brief Finds the attributes of an inode.
 *	\param path Absolute position of the inode.
 *	\return The requested attributes.
 */
int vfs_attr(char* path) {
  inode_t result = vfs_path_to_inode(NULL,path);
  if (errno > 0) {
    return 0;
  } else {
    return result.st.st_mode;
  }
}

int vfs_mkdir(char* path, char* name, int permissions) {
  inode_t position = vfs_path_to_inode(NULL,path);
  if (errno == 0) {
	  if (position.op->read_dir == NULL) {
		  errno = -1;
		  return -1;
	  }
	  vfs_dir_list_t* ls = position.op->read_dir(position);
	  vfs_dir_list_t* ls_start = ls;
	  if ((intptr_t)ls == -1) {
		  return 0;
	  }

	  bool found = false;
	  while (ls != NULL) {
		  if (strcmp(ls->name, name) == 0) {
			  found = true;
		  }
		  ls = ls->next;
	  }
	  free_vfs_dir_list(ls_start);
	  if (found) {
		  errno = EEXIST;
		  return 0;
	  }

	  if (position.op->mkdir == NULL) {
		  errno = -1;
		  return -1;
	  }
      return position.op->mkdir(position, name, permissions);
  } else {
      return 0;
  }
}

int vfs_rm(char* path, char* name) {
  inode_t position = vfs_path_to_inode(NULL, path);
  if (errno == 0) {
	  if (position.op->rm == NULL) {
		  errno = -1;
		  return -1;
	  }
      return position.op->rm(position, name);
  } else {
      return 0;
  }
}

int vfs_mkfile(char* path, char* name, int permissions) {
  inode_t position = vfs_path_to_inode(NULL,path);
  if (errno == 0) {
	  if (position.op->read_dir == NULL) {
		  errno = -1;
		  return -1;
	  }
	  vfs_dir_list_t* ls = position.op->read_dir(position);
	  vfs_dir_list_t* ls_start = ls;
	  if ((intptr_t)ls == -1) {
		  return 0;
	  }

	  bool found = false;
	  while (ls != NULL) {
		  if (strcmp(ls->name, name) == 0) {
			  found = true;
		  }
		  ls = ls->next;
	  }
	  free_vfs_dir_list(ls_start);
	  if (found) {
		  errno = EEXIST;
		  return 0;
	  }
	  if (position.op->mkfile == NULL) {
		  errno = -1;
		  return -1;
	  }
      return position.op->mkfile(position, name, permissions);
  } else {
      return 0;
  }
}
