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
      inode_t* r = vfs_path_to_inode(path);
      if (r == NULL) {
        kernel_printf("[INFO][VFS] shit happens.\n");
      } else {
          mount_points[nmounted].inode = r;
          mount_points[nmounted].fs = sb;
          nmounted++;
      }
  }
}

inode_t* vfs_path_to_inode(char *path_) {
  char* path = malloc(strlen(path_)+1);
  strcpy(path, path_);

  if (path[0] != '/') {
    free(path);
    errno = ENOENT;
    return 0;
  }

  kdebug(D_VFS, 1, "Resolving path %s\n", path);

  inode_t* position = root_inode;
  char* pch = path+1;
  char* old_pch = path+1;
  while ((pch = strchr(pch, '/')) != NULL) {
    pch[0] = 0;
    vfs_dir_list_t* result = position->op->read_dir(position);
    bool found = false;
    while (result != 0 && !found) {
        if (strcmp(old_pch, result->name) == 0 && (S_ISDIR(result->inode->st.st_mode))) {
            position = result->inode;
            found = true;
            for (int i=1;i<nmounted;i++) {
                kdebug(D_VFS, 1, "Mount transition: %s %d %d\n", old_pch, mount_points[i].inode->st.st_ino, result->inode->st.st_ino);
                if (result->inode->st.st_ino == mount_points[i].inode->st.st_ino && result->inode->sb->id == mount_points[i].inode->sb->id) {
                    position = mount_points[i].fs->root;
                }
            }
        }
        result = result->next;
    }

    if (!found) {
      kdebug(D_VFS, 3, "Bad path name: %s\n", path);
      free(path);
      errno = ENOENT;
      return 0;
    }

    old_pch = pch+1;
    pch = pch+1;
  }

  vfs_dir_list_t* result = position->op->read_dir(position);
  bool found = false;
  while (result != 0 && !found) {
    if (strcmp(old_pch, result->name) == 0) {
      position = result->inode;
      for (int i=1;i<nmounted;i++) {
          if (result->inode->st.st_ino == mount_points[i].inode->st.st_ino && result->inode->sb->id == mount_points[i].inode->sb->id) {
              position = mount_points[i].fs->root;
          }
      }
      found = true;
    }
    result = result->next;
  }

  if (!found && old_pch[0] != 0) {
    kdebug(D_VFS, 3, "ls: Bad path name: %s\n", path);
    free(path);
    errno = ENOENT;
    return 0;
  }

  kdebug(D_VFS, 1, "ls: %s -> %d\n", path, position->st.st_ino);
  free(path);

  inode_t* res = malloc(sizeof(inode_t));
  res->op       = position->op;
  res->sb       = position->sb;
  res->st       = position->st;
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
    inode_t* position = vfs_path_to_inode(path);
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
  inode_t* result = vfs_path_to_inode(path);
  if (result == 0) {
    return 0;
  } else {
    int attr = result->st.st_mode;
    free(result);
    return attr;
  }
}

int vfs_mkdir(char* path, char* name, int permissions) {
  inode_t* position = vfs_path_to_inode(path);
  if (position != 0) {
      int r = position->op->mkdir(position, name, permissions);
      free(position);
      return r;
  } else {
      return 0;
  }
}

int vfs_rm(char* path, char* name) {
  inode_t* position = vfs_path_to_inode(path);
  if (position != 0) {
      int r = position->op->rm(position, name);
      free(position);
      return r;
  } else {
      return 0;
  }
}

int vfs_mkfile(char* path, char* name, int permissions) {
  inode_t* position = vfs_path_to_inode(path);
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
