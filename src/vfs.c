#include "vfs.h"
#include <stdlib.h>
#include <errno.h>

inode_t* root_inode;
mount_point_t mount_points[20];
int nmounted = 0;


void vfs_setup() {}

void vfs_mount(superblock_t* sb, char* path) {
  if (nmounted == 0) {
    kernel_printf("[INFO][VFS] FS mounted.\n");
    root_inode = sb->root;
    nmounted++;
  } else {
      inode_t* r = vfs_path_to_inode(NULL,path);
      if (r == NULL) {
        kernel_printf("[INFO][VFS] shit happens.\n");
      } else {
          mount_points[nmounted].inode = r;
          mount_points[nmounted].fs = sb;
          nmounted++;
      }
  }
}

inode_t* vfs_parent(inode_t* base, int* ancestor, char* writebuf) {
	inode_t* mnt_root = mount_points[base->st.st_dev].fs->root;
	if (base->st.st_ino == mnt_root->st.st_ino) {
		// If this is the root of a mount point, teleport back.
		base = mount_points[base->st.st_dev].inode;
	}

	vfs_dir_list_t* result = base->op->read_dir(base);
	inode_t* res = 0;
	kernel_printf("%d\n", *ancestor);
	if (*ancestor == -1)
		writebuf[0] = 0;

	while(result) {
		if (strcmp(result->name, "..") == 0) {
			res = malloc(sizeof(inode_t));
			*res = *result->inode;
		}

		if (result->inode->st.st_ino == *ancestor) {
			strcpy(writebuf, result->name);
		}
		result = result->next;
	}
	*ancestor = base->st.st_ino;
	return res;
}

char* vfs_inode_to_path(inode_t* base, char* buf, size_t size) {
	int i=size-2;
	buf[i+1] = 0;
	buf[i] = '/';
	// i points to the beginning of the buffer.
	int ancestor = -1;
	inode_t* new_base;

	char tmp[255];
	while (base->st.st_ino != root_inode->st.st_ino || base->st.st_dev != root_inode->st.st_dev) {
		new_base = vfs_parent(base, &ancestor, tmp);
		if (strlen(tmp) > 0) {
			i -= strlen(tmp);
			memcpy(&buf[i], tmp, strlen(tmp));
			buf[i-1] = '/';
			i--;
		}
		free(base);
		base = new_base;
	}

	new_base = vfs_parent(base, &ancestor, tmp);
	if (strlen(tmp) > 0) {
		i -= strlen(tmp);
		memcpy(&buf[i], tmp, strlen(tmp));
		buf[i-1] = '/';
		i--;
	}
	free(new_base);
	free(base);
	for (int j=i;j<size;j++) {
		buf[j-i] = buf[j];
	}
	return buf;
}

//TODO: free entire vfs_dir_list_t.
inode_t* vfs_path_to_inode(inode_t* base, char *path_) {
  	char* path = malloc(strlen(path_)+1);
  	strcpy(path, path_);

	if (path[0] != '/' && base == NULL) {
		free(path);
		kdebug(D_VFS, 5, "Relative path but no base inode.\n");
		errno = ENOENT;
		return 0;
	}

	kdebug(D_VFS, 1, "Resolving path %s\n", path);

	inode_t position = *root_inode;
	if (path[0] != '/') // working on a relative path.
		position = *base;

	char* token = strtok(path, "/");
	if (token == NULL) {
		inode_t* res = malloc(sizeof(inode_t));
		res->op       = position.op;
		res->sb       = position.sb;
		res->st       = position.st;
		return res;
	}

	do {
		if (!S_ISDIR(position.st.st_mode)) {
			kdebug(D_VFS, 5, "Bad path name: %s failed on token %s\n", path_, token);
			free(path);
			errno = ENOENT;
			return NULL;
		}

		vfs_dir_list_t* result = position.op->read_dir(&position);
		bool found = false;
		while (result != 0 && !found) {
			if (strcmp(result->name, token) == 0) {
				position = *result->inode;
				kdebug(D_VFS, 1, "%s:%d\n",token,result->inode->st.st_ino);
				found = true;
				for (int i=1;i<nmounted;i++) {
	                kdebug(D_VFS, 1, "Mount transition: %s %d %d\n", token, mount_points[i].inode->st.st_ino, result->inode->st.st_ino);
					if (result->inode->st.st_ino == mount_points[i].inode->st.st_ino && result->inode->sb->id == mount_points[i].inode->sb->id) {
	                    position = *mount_points[i].fs->root;
	                }
				}
			}

			vfs_dir_list_t* prec = result;
			result = result->next;
			free(prec->inode);
			free(prec->name);
			free(prec);
		}

		if (!found) {
			kdebug(D_VFS, 5, "Bad path name: %s failed on token %s\n", path_, token);
			free(path);
			errno = ENOENT;
			return NULL;
		}
	} while ((token = strtok(NULL, "/")) != NULL);

	kdebug(D_VFS, 3, "path_to_ino: %s -> %d\n", path_, position.st.st_ino);
	free(path);

	inode_t* res = malloc(sizeof(inode_t));
	res->op       = position.op;
	res->sb       = position.sb;
	res->st       = position.st;
	return res;
}

int vfs_fwrite(inode_t* fd, char* buffer, int length, int offset) {
    if (S_ISDIR(fd->st.st_mode)) {
        kernel_printf("vfs_fwrite: whoa there.. this a directory..\n");
        return 0;
    }

    if (offset > fd->st.st_size) {
        offset = fd->st.st_size;
        kernel_printf("vfs_fwrite: offset > size.\n");
    }

    return fd->op->write(fd, buffer, length, offset);
}

int vfs_fread(inode_t* fd, char* buffer, int length, int offset) {
  if (S_ISDIR(fd->st.st_mode)) {
    kernel_printf("vfs_fread: whoa there.. this a directory..\n");
    return 0;
  }


  if (offset > fd->st.st_size) {
      offset = fd->st.st_size;
      kernel_printf("vfs_fread: offset > size.\n");
  }

  return fd->op->read(fd, buffer, length, offset);
}

vfs_dir_list_t* vfs_readdir(char* path) {
    inode_t* position = vfs_path_to_inode(NULL, path);
    if (position != 0) {
        vfs_dir_list_t* r = position->op->read_dir(position);
        free(position);
        return r;
    }
    else {
        return 0;
    }
}

int vfs_attr(char* path) {
  inode_t* result = vfs_path_to_inode(NULL,path);
  if (result == 0) {
    return 0;
  } else {
    int attr = result->st.st_mode;
    free(result);
    return attr;
  }
}

int vfs_mkdir(char* path, char* name, int permissions) {
  inode_t* position = vfs_path_to_inode(NULL,path);
  if (position != 0) {
      int r = position->op->mkdir(position, name, permissions);
      free(position);
      return r;
  } else {
      return 0;
  }
}

int vfs_rm(char* path, char* name) {
  inode_t* position = vfs_path_to_inode(NULL, path);
  if (position != 0) {
      int r = position->op->rm(position, name);
      free(position);
      return r;
  } else {
      return 0;
  }
}

int vfs_mkfile(char* path, char* name, int permissions) {
  inode_t* position = vfs_path_to_inode(NULL,path);
  if (position != 0) {
      int r = position->op->mkfile(position, name, permissions);
      free(position);
      return r;
  } else {
      return 0;
  }
}

perm_str_t vfs_permission_string(int attr) {
  perm_str_t r;
  r.str[10] = 0;
  for (int i=0;i<10;i++)
    r.str[i] = '-';

  if (S_ISDIR(attr))
    r.str[0] = 'd';
  if (S_ISCHR(attr))
    r.str[0] = 'c';
  if (S_ISBLK(attr))
    r.str[0] = 'b';

  if (attr & S_IRUSR)
    r.str[1] = 'r';
  if (attr & S_IWUSR)
    r.str[2] = 'w';
  if (attr & S_IXUSR)
    r.str[3] = 'x';

  if (attr & S_IRGRP)
    r.str[4] = 'r';
  if (attr & S_IWGRP)
    r.str[5] = 'w';
  if (attr & S_IXGRP)
    r.str[6] = 'x';

  if (attr & S_IROTH)
    r.str[7] = 'r';
  if (attr & S_IWOTH)
    r.str[8] = 'w';
  if (attr & S_IXOTH)
    r.str[9] = 'x';
  return r;
}
