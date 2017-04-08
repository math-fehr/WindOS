#include "vfs.h"
#include <stdlib.h>

inode_t root_inode;
mount_point_t mount_points[20];
int nmounted = 0;

void vfs_setup() {}

inode_t vfs_path_to_inode(char *path_) {
  char* path = malloc(strlen(path_)+1);
  strcpy(path, path_);

  inode_t dummy;
  dummy.number = 0;

  if (path[0] != '/') {
    free(path);
  //  kernel_printf("[ERROR][VFS] Bad path name: %s\n", path);
    return dummy;
  }

  //kernel_printf("[INFO][VFS] Resolving path %s\n", path);

  inode_t position = root_inode;
  char* pch = path+1;
  char* old_pch = path+1;
  while ((pch = strchr(pch, '/')) != NULL) {
    pch[0] = 0;
    vfs_dir_list_t* result = position.op->read_dir(position.sb, position);
    bool found = false;
    while (result != 0 && !found) {
      if (strcmp(old_pch, result->name) == 0 && (result->inode.attr & VFS_DIRECTORY)) {
        position = result->inode;
        found = true;
      }
      result = result->next;
    }

    if (!found) {
    //  kernel_printf("[ERROR][VFS] Bad path name: %s\n", path);

      free(path);
      return dummy;
    }

    old_pch = pch+1;
    pch = pch+1;
  }

  vfs_dir_list_t* result = position.op->read_dir(position.sb, position);
  bool found = false;
  while (result != 0 && !found) {
    if (strcmp(old_pch, result->name) == 0) {
      position = result->inode;
      found = true;
    }
    result = result->next;
  }

  if (!found && old_pch[0] != 0) {
    free(path);
    //kernel_printf("[ERROR][VFS] Bad path name: %s\n", path);
    return dummy;
  }

  free(path);
  //kernel_printf("[INFO][VFS] %s -> %d\n", path, position.number);
  return position;
}

void vfs_mount(superblock_t* sb, char* path) {
  if (nmounted == 0) {
    kernel_printf("[INFO][VFS] FS mounted.\n");
    root_inode = sb->root;
  } else {
    mount_points[nmounted].inode = vfs_path_to_inode(path);
    mount_points[nmounted].fs = sb;
    nmounted++;
  }
}


int vfs_fopen(char* path, int mode) {
  return 0;
}

int vfs_fclose(int fd) {
  return 0;
}

int vfs_fseek(int fd, int offset) {
  return 0;
}

int vfs_fwrite(int fd, char* buffer, int length) {
  return 0;
}

int vfs_fread(int fd, char* buffer, int length) {
  return 0;
}

vfs_dir_list_t* vfs_readdir(char* path) {
  inode_t position = vfs_path_to_inode(path);
  return position.op->read_dir(position.sb, position);
}

int vfs_permissions(char* path) {
  return 0;
}

void vfs_mkdir(char* path, char* name, int permissions) {

}

void vfs_rmdir(char* path, char* name) {

}

void vfs_mkfile(char* path, char* name, int permissions) {

}

void vfs_rmfile(char* path, char* name) {

}

perm_str_t vfs_permission_string(int attr) {
  perm_str_t r;
  r.str[10] = 0;
  for (int i=0;i<10;i++)
    r.str[i] = '-';

  if (attr & VFS_DIRECTORY)
    r.str[0] = 'd';

  if (attr & VFS_RUSR)
    r.str[1] = 'r';
  if (attr & VFS_WUSR)
    r.str[2] = 'w';
  if (attr & VFS_XUSR)
    r.str[3] = 'x';

  if (attr & VFS_RGRP)
    r.str[4] = 'r';
  if (attr & VFS_WGRP)
    r.str[5] = 'w';
  if (attr & VFS_XGRP)
    r.str[6] = 'x';

  if (attr & VFS_ROTH)
    r.str[7] = 'r';
  if (attr & VFS_WOTH)
    r.str[8] = 'w';
  if (attr & VFS_XOTH)
    r.str[9] = 'x';
  return r;
}
