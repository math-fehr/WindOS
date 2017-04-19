#include "vfs.h"
#include <stdlib.h>

inode_t root_inode;
mount_point_t mount_points[20];
int nmounted = 0;

file_t file_table[VFS_MAX_OPEN_FILES];
bool open_file_bitmap[VFS_MAX_OPEN_FILES];

inode_t inode_table[VFS_MAX_OPEN_INODES];
char open_inode_ref_cnt[VFS_MAX_OPEN_INODES];

void vfs_setup() {
  for (int i=0; i<VFS_MAX_OPEN_FILES;i++) {
    open_file_bitmap[i] = false;
  }

  for (int i=0; i<VFS_MAX_OPEN_INODES;i++) {
    open_inode_ref_cnt[i] = 0;
  }
}

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

  kdebug(D_VFS, 1, "Resolving path %s\n", path);

  inode_t position = root_inode;
  char* pch = path+1;
  char* old_pch = path+1;
  while ((pch = strchr(pch, '/')) != NULL) {
    pch[0] = 0;
    vfs_dir_list_t* result = position.op->read_dir(position);
    bool found = false;
    while (result != 0 && !found) {
      if (strcmp(old_pch, result->name) == 0 && (result->inode.attr & VFS_DIRECTORY)) {
        position = result->inode;
        found = true;
      }
      result = result->next;
    }

    if (!found) {
      kdebug(D_VFS, 3, "Bad path name: %s\n", path);
      free(path);
      return dummy;
    }

    old_pch = pch+1;
    pch = pch+1;
  }

  vfs_dir_list_t* result = position.op->read_dir(position);
  bool found = false;
  while (result != 0 && !found) {
    if (strcmp(old_pch, result->name) == 0) {
      position = result->inode;
      found = true;
    }
    result = result->next;
  }

  if (!found && old_pch[0] != 0) {
    kdebug(D_VFS, 3, "ls: Bad path name: %s\n", path);
    free(path);
    return dummy;
  }

  kdebug(D_VFS, 1, "ls: %s -> %d\n", path, position.number);
  free(path);
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


int vfs_fopen(char* path) {
  int i;
  for (i=0; i<VFS_MAX_OPEN_FILES && open_file_bitmap[i];i++) {}
  if (i == VFS_MAX_OPEN_FILES) {
    kernel_printf("[ERROR][VFS] vfs_fopen: file number limit reached.");
    return -1;
  }

  open_file_bitmap[i] = true;

  inode_t result = vfs_path_to_inode(path);

  if (result.number == 0) {
    kernel_printf("[ERROR][VFS] vfs_fopen: no such file or directory.");
    return -1;
  }

  int inode_i;
  int j=VFS_MAX_OPEN_INODES;
  // search if the inode has already been opened.
  for (inode_i=0; inode_i<VFS_MAX_OPEN_INODES && (open_inode_ref_cnt[inode_i] == 0 || inode_table[inode_i].number != result.number); inode_i++) {
    if (open_inode_ref_cnt[inode_i] == 0) j=inode_i;
  }

  if (inode_i==VFS_MAX_OPEN_INODES) {
    if (j==VFS_MAX_OPEN_INODES) {
      kernel_printf("[ERROR][VFS] vfs_fopen: inode number limit reached.");
      return -1;
    } else {
      inode_table[j] = result;
      inode_i = j;
    }
  }

  open_inode_ref_cnt[inode_i]++;

  file_table[i].current_offset = 0;
  file_table[i].inode = &inode_table[inode_i];
  return i;
}

int vfs_fclose(int fd) {
  if (open_file_bitmap[fd]) {
    int inode_i = (file_table[fd].inode - inode_table)/sizeof(inode_t);
    open_inode_ref_cnt[inode_i]--;
    open_file_bitmap[fd] = false;
    return 1;
  }
  return 0;
}

int vfs_fseek(int fd, int offset) {
  if (!open_file_bitmap[fd]) return 0;

  int size = file_table[fd].inode->size;
  int cur_ofs = file_table[fd].current_offset;
  int pos = cur_ofs + offset;
  if (pos < 0) pos = 0;
  if (pos > size) pos = size;
  file_table[fd].current_offset = pos;

  return pos - cur_ofs;
}

int vfs_fmove(int fd, int position) {
  if (!open_file_bitmap[fd]) return 0;

  int size = file_table[fd].inode->size;
  int pos = position;
  if (pos < 0) pos = 0;
  if (pos > size) pos = size;
  file_table[fd].current_offset = pos;

  return pos;
}

int vfs_fwrite(int fd, char* buffer, int length) {
  if (!open_file_bitmap[fd]) return 0;
  file_t f = file_table[fd];

  if (!(f.inode->attr & VFS_FILE)) {
    kernel_printf("vfs_fwrite: whoa there.. this a directory man..\n");
    return 0;
  }

  int nwritten = file_table[fd].inode->op->write(*f.inode, buffer, length, f.current_offset);
  f.current_offset += nwritten;
  f.inode->size = max(f.current_offset, f.inode->size);
  return 0;
}

int vfs_fread(int fd, char* buffer, int length) {
  if (!open_file_bitmap[fd]) return 0;
  file_t* f = &file_table[fd];

  if (!(f->inode->attr & VFS_FILE)) {
    kernel_printf("vfs_fread: whoa there.. this a directory man..\n");
    return 0;
  }

  int nread = f->inode->op->read(*f->inode, buffer, length, f->current_offset);
  f->current_offset += nread;
  return nread;
}

vfs_dir_list_t* vfs_readdir(char* path) {
  inode_t position = vfs_path_to_inode(path);
  if (position.number > 0)
    return position.op->read_dir(position);
  else
    return 0;
}

int vfs_attr(char* path) {
  inode_t result = vfs_path_to_inode(path);
  if (result.number == 0) {
    return 0;
  } else {
    return result.attr;
  }
}

int vfs_mkdir(char* path, char* name, int permissions) {
  inode_t position = vfs_path_to_inode(path);
  return position.op->mkdir(position, name, permissions);
}

int vfs_rm(char* path, char* name) {
  inode_t position = vfs_path_to_inode(path);
  return position.op->rm(position, name);
}

int vfs_mkfile(char* path, char* name, int permissions) {
  inode_t position = vfs_path_to_inode(path);
  return position.op->mkfile(position, name, permissions);
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
