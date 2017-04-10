#include "ext2.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

ext2_device_t devices[10];
int i = 0; 

static inode_operations_t ext2_inode_operations = {
  .read_dir = ext2_lsdir,
  .read = ext2_fread,
  .write = ext2_fwrite,
  .mkdir = ext2_mkdir,
  .rm = ext2_rm,
  .mkfile = ext2_mkfile,
};

superblock_t* ext2fs_initialize(storage_driver* disk) {
  ext2_superblock_t* sb = (ext2_superblock_t*) malloc(sizeof(ext2_superblock_t));
  disk->read(1024, sb, sizeof(ext2_superblock_t));

  if (sb->signature != 0xef53) {
    kernel_printf("[ERROR][EXT2] Could not setup filesystem: wrong signature.\n");
    kernel_printf("                0xEF53 != %#04X\n", sb->signature);
    free(sb);
    return NULL;
  }
  kernel_printf("[INFO][EXT2] Signature found. Loading filesystem superblock.\n");

  ext2_extended_superblock_t* esb = (ext2_extended_superblock_t*) malloc(sizeof(ext2_extended_superblock_t));

  //print_hex(sb->major_version,1);
  //print_hex(sb->minor_version,1);
  kernel_printf("[INFO][EXT2] Version is %d.%d\n", sb->major_version, sb->minor_version);
  if (sb->major_version >= 1) {
    disk->read(1024 + 84, esb, sizeof(ext2_extended_superblock_t));
  } else {
    esb->first_non_reserved_inode = 11;
    esb->inode_size = 128;
    esb->optional_features = 0;
    esb->readonly_features = 0;
    esb->required_features = 0;
  }

  devices[i].disk = disk;
  devices[i].esb = esb;
  devices[i].sb = sb;

  superblock_t* result = (superblock_t*) malloc(sizeof(superblock_t));
  result->id = i;
  result->root.number = 2;
  result->root.sb = result;
  result->root.op = &ext2_inode_operations;
  result->root.size = ext2_get_inode_descriptor(result, 2).size;
  i++;
  return result;
}



ext2_block_group_descriptor_t ext2_get_block_group_descriptor(superblock_t* fs, int bg) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  ext2_block_group_descriptor_t group_descriptor;
  int block_size = 1024 << sb->log_block_size;
  int bgdt = 2048;
  if (sb->log_block_size > 0) {
    bgdt = block_size;
  }
  int address = bgdt + 32*bg;
  disk->read(address, &group_descriptor, sizeof(ext2_block_group_descriptor_t));

  return group_descriptor;
}

void ext2_set_block_group_descriptor(superblock_t* fs, ext2_block_group_descriptor_t data, int bg) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int bgdt = 2048;
  if (sb->log_block_size > 0) {
    bgdt = block_size;
  }
  int address = bgdt + 32*bg;
  disk->write(address, &data, sizeof(ext2_block_group_descriptor_t));

}

ext2_inode_t ext2_get_inode_descriptor(superblock_t* fs, int inode) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  ext2_extended_superblock_t* esb = devices[fs->id].esb;
  storage_driver* disk = devices[fs->id].disk;

  int block_group = (inode-1) / sb->inodes_per_group;
  int block_size = 1024 << sb->log_block_size;

  ext2_block_group_descriptor_t group_descriptor =
    ext2_get_block_group_descriptor(fs, block_group);

  int index = (inode-1) % sb->inodes_per_group;

  ext2_inode_t inode_descriptor;
  int address = group_descriptor.inode_table_addr * block_size + index * esb->inode_size;
  disk->read(address, &inode_descriptor, sizeof(inode_descriptor));
  return inode_descriptor;
}



uintptr_t ext2_get_block_address(superblock_t* fs, ext2_inode_t inode, int block) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int ind_size = block_size / 4;
  uintptr_t base_address = 0;

  if (block < 12) {
    base_address = inode.direct_block_ptr[block];
  } else if (block < 12+ind_size) { // singly indirect block
    int block_1 = block - 12;
    // Address of the first block.
    uintptr_t s_ind_block =  (uintptr_t) inode.singly_indirect_block_ptr;
    disk->read(s_ind_block + 4*block_1, &base_address, 4);
  } else if (block < 12+ind_size+ind_size*ind_size) {// doubly indirect block
    block = block - 12 - ind_size;
    int block_1 = block / ind_size;
    int block_2 = block % ind_size;
    // Address of the first block;
    uintptr_t d_ind_block = (uintptr_t) inode.doubly_indirect_block_ptr;
    uintptr_t s_ind_block = 0;
    disk->read(d_ind_block + 4*block_1, &s_ind_block, 4);
    disk->read(s_ind_block + 4*block_2, &base_address, 4);
  } else { // triply indirect block
    block = block - 12 - ind_size - ind_size*ind_size;
    int block_1 = block / (ind_size*ind_size);
    int block_2 = (block % (ind_size*ind_size)) / ind_size;
    int block_3 = block % ind_size;
    uintptr_t t_ind_block = (uintptr_t) inode.triply_indirect_block_ptr;
    uintptr_t d_ind_block = 0;
    uintptr_t s_ind_block = 0;
    // From the first block, reads the address of the second block.
    disk->read(t_ind_block + 4*block_1, &d_ind_block, 4);
    // From the second block, reads the address of the third block.
    disk->read(d_ind_block + 4*block_2, &s_ind_block, 4);
    // From the third block, reads the address of the data block.
    disk->read(s_ind_block + 4*block_3, &base_address, 4);
  }
  return base_address;
}

// reads from a block: offset < block_size and offset+size <= block_size
void ext2_inode_read_block(superblock_t* fs, ext2_inode_t inode, char* buffer, int block, int offset, int size) {
  //kernel_printf("[INFO][EXT2] Reading block %d for inode from %d to %d\n", block, offset, offset+size);
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  uintptr_t base_address = ext2_get_block_address(fs, inode, block);
  disk->read(base_address*block_size+offset, buffer, size);
}



int ext2_fread( inode_t vfs_inode, char* buffer, int size, int position) {
  superblock_t* fs = vfs_inode.sb;
  int inode = vfs_inode.number;

  //kernel_printf("[INFO][EXT2] Reading file inode %d\n", inode);
  ext2_superblock_t* sb = devices[fs->id].sb;

  ext2_inode_t info = ext2_get_inode_descriptor(fs, inode);
  if (!(info.type_permissions & EXT2_INODE_FILE)) {
    kernel_printf("[INFO][EXT2] Inode %d is not a file.\n", inode);
    return 0;
  }

  if (position + size > info.size) {
    size = info.size - position;
  }

  int block_size = 1024 << sb->log_block_size;
  int first_block = position / block_size;
  int last_block = (position+size-1) / block_size;

  int offset = position % block_size;

  if (offset+size <= block_size) { // data on single block
    ext2_inode_read_block(fs, info, buffer, first_block, offset, size);
  } else { // data on several blocks
    ext2_inode_read_block(fs, info, buffer, first_block, offset, block_size-offset);
    int buffer_position = block_size - offset;
    for (int block=first_block+1; block < last_block; block++) {
      ext2_inode_read_block(fs, info, buffer+buffer_position, block, 0, block_size);
      buffer_position += block_size;
    }
    ext2_inode_read_block(fs, info, buffer+buffer_position, last_block, 0, size-buffer_position);
  }
  buffer[size] = 0;
  return size;
}


 // |inode(4)|size(2)|length(1)|type(1)|name(N)
void ext2_add_dir_entry(superblock_t* fs, int dir, int dest, char* name) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  int block_size = 1024 << sb->log_block_size;

  int new_entry_size = 8+strlen(name);

  ext2_inode_t data = ext2_get_inode_descriptor(fs, dir);

  int last_entry_size;
  int pos;
  char* databuf = malloc(block_size);
  int last_block = (data.size-1)/block_size;

  if (data.size == 0)  {
    pos = 0;
    last_entry_size = block_size;
  } else {
    ext2_inode_read_block(fs, data, databuf, last_block, 0, block_size);
    pos = 0;
    uint16_t entry_size = 0;

    do {
      pos += entry_size;
      entry_size = databuf[pos+4] + (databuf[pos+5] << 8);
    } while (pos+entry_size < block_size);
    // now pos refers to the last directory entry.
    last_entry_size = 8+databuf[pos+6];
  }

  char* entry = malloc(new_entry_size);
  memcpy(entry, &dest, 4);
  entry[6] = strlen(name);
  entry[7] = EXT2_ENTRY_DIRECTORY;
  strcpy(entry+8, name);
  if (new_entry_size > block_size-last_entry_size-pos) { // create a new block
    uint16_t w_size = block_size;
    memcpy(entry+4, &w_size, 2);

    ext2_append_file(fs, dir, entry, new_entry_size);
    int n_size = data.size+block_size;
    data = ext2_get_inode_descriptor(fs, dir);
    data.size = n_size;
    ext2_update_inode_data(fs, dir, data);
  } else { // append and update previous entry.
    ext2_replace_file(fs, dir, (char*)&last_entry_size, 2, 4+pos+last_block*block_size);

    uint16_t w_size = block_size-last_entry_size-pos;
    memcpy(entry+4, &w_size, 2);

    ext2_replace_file(fs, dir, entry, new_entry_size, last_entry_size+pos+last_block*block_size);
  }

  free(databuf);
  free(entry);
}

int ext2_mkdir (inode_t inode, char* name, int perm) {
  int new_dir = ext2_get_free_inode(inode.sb);
  ext2_register_inode(inode.sb, new_dir);
  ext2_inode_t data;
  data.creation_time = timer_get_posix_time();
  data.last_access_time = timer_get_posix_time();
  data.group_id = 0;
  data.size = 0;
  data.user_id = 0;
  data.type_permissions = EXT2_INODE_DIRECTORY | perm;
  data.last_modification_time = timer_get_posix_time();
  data.hard_links = 1;
  for (int i=0;i<12;i++) data.direct_block_ptr[i] = 0;
  data.singly_indirect_block_ptr = 0;
  data.doubly_indirect_block_ptr = 0;
  data.triply_indirect_block_ptr = 0;
  ext2_update_inode_data(inode.sb, new_dir, data);

  ext2_add_dir_entry(inode.sb, new_dir, new_dir, ".");
  ext2_add_dir_entry(inode.sb, new_dir, inode.number, "..");

  ext2_add_dir_entry(inode.sb, inode.number, new_dir, name);
  return 1;
}


int ext2_rm (inode_t inode, char* name) {
  superblock_t* fs = inode.sb;
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;
  int block_size = 1024 << sb->log_block_size;

  uintptr_t addr = 0;
  int current_block = 0;
  int rm_inode = 0;

  int n = strlen(name);

  ext2_inode_t container_desc = ext2_get_inode_descriptor(fs, inode.number);
  char* data = malloc(block_size);

  bool found = false;
  bool deleted = false;

  while ((!found) && (addr = ext2_get_block_address(fs, container_desc, current_block)) != 0) {
    disk->read(addr*block_size, data, block_size);
    int explorer = 0;
    while (explorer < block_size) {
      int entry_size = data[explorer+4] + (data[explorer+5] << 8);
      int name_size = data[explorer+6];
      if (name_size == n && (0 == strncmp(&data[explorer+8], name, n))) {
        found = true;
        int deleted_inode=0;
        memcpy(&deleted_inode, &data[explorer], 4);
        ext2_inode_t desc = ext2_get_inode_descriptor(fs, deleted_inode);
        bool ok_to_delete = true;
        if (desc.type_permissions & EXT2_INODE_DIRECTORY) {
          inode_t t;
          t.number = deleted_inode;
          t.sb = fs;
          vfs_dir_list_t* lst = ext2_lsdir(t);
          int cnt = 0;
          while (lst != NULL) {
            if(strcmp(".",lst->name) != 0 && strcmp("..",lst->name) != 0) {
              cnt++;
            }
            lst = lst->next;
          }
          if (cnt > 0) {
            kernel_printf("[ERROR][VFS] Directory isn't empty. (%d)\n",cnt);
            ok_to_delete = false;
          }
        }
        if (ok_to_delete) { // mark the inode as deleted.
          int zero = 0;
          disk->write(addr*block_size+explorer, &zero, 4);
          deleted = true;
          rm_inode = deleted_inode;
        }
      }
      explorer += entry_size;
    }
    current_block++;
  }

  // We have to free the inode and blocks.
  if (deleted) {
    ext2_free_inode_blocks(fs, rm_inode);
    ext2_free_inode(fs, rm_inode);
    return 1;
  }

  return 0;
}

int ext2_mkfile (inode_t inode, char* name, int perm) {
  int new_file = ext2_get_free_inode(inode.sb);
  ext2_register_inode(inode.sb, new_file);
  ext2_inode_t data;
  data.creation_time = timer_get_posix_time();
  data.last_access_time = timer_get_posix_time();
  data.group_id = 0;
  data.size = 0;
  data.user_id = 0;
  data.type_permissions = EXT2_INODE_FILE | perm;
  data.last_modification_time = timer_get_posix_time();
  data.hard_links = 1;
  for (int i=0;i<12;i++) data.direct_block_ptr[i] = 0;
  data.singly_indirect_block_ptr = 0;
  data.doubly_indirect_block_ptr = 0;
  data.triply_indirect_block_ptr = 0;
  ext2_update_inode_data(inode.sb, new_file, data);

  ext2_add_dir_entry(inode.sb, inode.number, new_file, name);
  return 0;
}

/*
 * List the child elements of an inode.
 */
vfs_dir_list_t* ext2_lsdir(inode_t inode_p) {
  superblock_t* fs = inode_p.sb;
  ext2_superblock_t* sb = devices[fs->id].sb;
  //ext2_extended_superblock_t* esb = devices[fs->id].esb;
  storage_driver* disk = devices[fs->id].disk;

  int inode = inode_p.number;

  vfs_dir_list_t* dir_list = 0;
  ext2_inode_t result = ext2_get_inode_descriptor(fs, inode);
  if (!(result.type_permissions & EXT2_INODE_DIRECTORY)) {
    kernel_printf("[INFO][EXT2] Inode %d is not a directory.", inode);
    return 0;
  }

  int block_size = 1024 << sb->log_block_size;
  uint8_t* data =  (uint8_t*) malloc(block_size);
  for (int i=0; i<12; i++) {
    if (result.direct_block_ptr[i] != 0) { // There is data in the block
      disk->read(result.direct_block_ptr[i]*block_size, data, block_size);
      int explorer = 0;
      while (explorer < block_size) {
        int entry_size = data[explorer+4] + (data[explorer+5] << 8);
        uint32_t l_inode = 0;
        memcpy(&l_inode, &data[explorer], 4);
        if (l_inode == 0) {
          if (entry_size == 0) {
            explorer = block_size;
          } else {
            explorer += entry_size;
          }
        } else {
          uint8_t length = data[explorer+6];
          char* name = (char*) malloc(length+1);
          name[length] = 0;
          memcpy(name, &data[explorer+8], length);
          vfs_dir_list_t* entry = malloc(sizeof(vfs_dir_list_t));
          entry->next = dir_list;
          entry->name = name;
          entry->inode.sb = fs;
          entry->inode.number = l_inode;
          entry->inode.op = &ext2_inode_operations;
          ext2_inode_t r = ext2_get_inode_descriptor(fs, l_inode);
          entry->inode.attr = r.type_permissions;
          entry->inode.size = r.size;

          dir_list = entry;

          explorer += entry_size;
        }
      }
    }
  }
  free(data);
  return dir_list;
}

// Returns the first available inode
int ext2_get_free_inode(superblock_t* fs) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  ext2_block_group_descriptor_t group_descriptor;
  int block_size = 1024 << sb->log_block_size;

  for (int i=0; i<sb->total_block_count;i++) {
    group_descriptor = ext2_get_block_group_descriptor(fs, i);
    if (group_descriptor.unallocated_inode_count > 0) {
      uint8_t* bitmap = malloc(block_size);
      disk->read(group_descriptor.inode_usage_bitmap_addr*block_size, bitmap, block_size);
      int found = -1;
      for (int b=0;b<block_size && found == -1;b++) {
        if (bitmap[b] != 0xFF) {
          for (int bit=0;bit<8;bit++) {
            if (!(bitmap[b] & (1 << bit))) { // bit is 0
              found = b*8 + bit;
            }
          }
        }
      }
      free(bitmap);
      return 1 + found + sb->inodes_per_group*i;
    }
  }
  kernel_printf("[INFO][EXT2] ext2_get_free_inode: error. No available inode.");
  return 0;
}

// Returns the first available block
int ext2_get_free_block(superblock_t* fs) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  ext2_block_group_descriptor_t group_descriptor;
  int block_size = 1024 << sb->log_block_size;

  for (int i=0; i<sb->total_block_count;i++) {
    group_descriptor = ext2_get_block_group_descriptor(fs, i);
    if (group_descriptor.unallocated_inode_count > 0) {
      uint8_t* bitmap = malloc(block_size);
      disk->read(group_descriptor.block_usage_bitmap_addr*block_size, bitmap, block_size);
      int found = -1;
      for (int b=0;b<block_size && found == -1;b++) {
        if (bitmap[b] != 0xFF) {
          for (int bit=0;bit<8;bit++) {
            if (!(bitmap[b] & (1 << bit))) { // bit is 0
              found = b*8 + bit;
            }
          }
        }
      }
      free(bitmap);
      return found + sb->blocks_per_group*i;
    }
  }
  kernel_printf("[INFO][EXT2] ext2_get_free_block: error. No available block.");
  return -1;
}

/*
 * Assume position is the position of a valid block that has been registered.
 *  if level = 0, just link the b-th block to the b_address
 *  else compute in which subblock should we go, create it if necessary and
 *   go in depth.
 */
void recursive_setup_block(superblock_t* fs, int position, int b, int b_address, int level) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;
  int block_size = 1024 << sb->log_block_size;
  int ind_size = block_size / 4;

  if (level == 0) {
    disk->write(position*block_size + b*4, &b_address, 4);
  } else {
    int b_base = b / ipow(ind_size, level);
    int b_rem  = b % ipow(ind_size, level);
    int next_addr;
    disk->read(position*block_size + b_base*4, &next_addr, 4);

    if (next_addr == 0) {
      next_addr = ext2_get_free_block(fs);
      ext2_register_block(fs, next_addr);
      disk->write(position*block_size + b_base*4, &next_addr, 4);
    }
    recursive_setup_block(fs, next_addr, b_rem, b_address, level-1);
  }
}


void ext2_set_inode_block_address(superblock_t* fs, int inode, int b, int b_address) {
  ext2_superblock_t* sb = devices[fs->id].sb;

  int block_size = 1024 << sb->log_block_size;
  ext2_inode_t data = ext2_get_inode_descriptor(fs, inode);
  int ind_size = block_size / 4;

  if (b < 12) {
    data.direct_block_ptr[b] = b_address;
  } else if(b < 12 + ind_size) {
    if (data.singly_indirect_block_ptr == 0) {
      data.singly_indirect_block_ptr = ext2_get_free_block(fs);
      ext2_register_block(fs, data.singly_indirect_block_ptr);
    }
    recursive_setup_block(fs, data.singly_indirect_block_ptr, b-12, b_address, 0);
  } else if(b < 12 + ind_size + ind_size * ind_size) {
    if (data.doubly_indirect_block_ptr == 0) {
      data.doubly_indirect_block_ptr = ext2_get_free_block(fs);
      ext2_register_block(fs, data.doubly_indirect_block_ptr);
    }
    recursive_setup_block(fs, data.doubly_indirect_block_ptr, b-12-ind_size, b_address, 1);
  } else {
    if (data.triply_indirect_block_ptr == 0) {
      data.triply_indirect_block_ptr = ext2_get_free_block(fs);
      ext2_register_block(fs, data.triply_indirect_block_ptr);
    }
    recursive_setup_block(fs, data.triply_indirect_block_ptr, b-12-ind_size-ind_size*ind_size, b_address, 2);
  }

  ext2_update_inode_data(fs, inode, data);
}


int ext2_fwrite(inode_t inode, char* buf, int len, int ofs) {
  superblock_t* fs = inode.sb;

  ext2_inode_t data = ext2_get_inode_descriptor(fs, inode.number);

  int to_append = max(len+ofs - data.size, 0);
  int to_replace = len - to_append;

  ext2_replace_file(fs, inode.number, buf, to_replace, ofs);
  ext2_append_file(fs, inode.number, buf+to_replace, to_append);
  return len;
}

void ext2_replace_file(superblock_t* fs, int inode, char* buffer, int size, int ofs) {
    ext2_superblock_t* sb = devices[fs->id].sb;
    storage_driver* disk = devices[fs->id].disk;

    int block_size = 1024 << sb->log_block_size;
    ext2_inode_t data = ext2_get_inode_descriptor(fs, inode);

    int first_block = ofs/block_size;
    uintptr_t first_block_address = ext2_get_block_address(fs, data, first_block);
    int last_block = (ofs+size)/block_size;

    int buf_pos = 0;
    int first_blk_write = min(size, block_size - (ofs%block_size));
    disk->write(first_block_address*block_size + ofs%block_size, buffer, first_blk_write);
    buf_pos = first_blk_write;

    if (first_block == last_block) return;

    for (int b=first_block+1; b<last_block; b++) {
      int block_address = ext2_get_block_address(fs, data, b);
      disk->write(block_address*block_size, buffer+buf_pos, block_size);
      buf_pos += block_size;
    }

    uintptr_t last_block_address = ext2_get_block_address(fs, data, last_block);
    disk->write(last_block_address*block_size, buffer+buf_pos, size-buf_pos);
}

void ext2_append_file(superblock_t* fs, int inode, char* buffer, int size) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  ext2_inode_t data = ext2_get_inode_descriptor(fs, inode);

  int fill_blk = block_size - (data.size % block_size);
  if (fill_blk == block_size)
    fill_blk = 0;
  if (fill_blk > size) {
    fill_blk = size;
  }

  int n_blocks_to_create = (size - fill_blk + block_size - 1)/block_size;
  int last_block = ((int)data.size-1)/block_size;

  if (data.size == 0)
    last_block = -1;

  if (last_block != -1) {
    uintptr_t last_block_address = ext2_get_block_address(fs, data, last_block);
    disk->write(last_block_address*block_size, buffer, fill_blk);
  }

  int buf_pos = fill_blk; // position in the buffer.

  for (int b = last_block+1; b < last_block+1+n_blocks_to_create; b++) {
    int b_address = ext2_get_free_block(fs);
    ext2_register_block(fs, b_address);
    ext2_set_inode_block_address(fs, inode, b, b_address);
    disk->write(b_address*block_size, buffer + buf_pos, min(size-buf_pos, block_size));
    buf_pos += block_size;
  }

  data = ext2_get_inode_descriptor(fs, inode);
  data.size += size;
  ext2_update_inode_data(fs, inode, data);
}

void recursive_block_delete(superblock_t* fs, int block_address, int level) {
  if (block_address == 0)
    return;

  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int ind_size = block_size / 4;

  // We only need to deallocate the pointed block.
  if (level > 0)  {
    uint32_t* buffer = malloc(block_size);
    disk->read(block_address*block_size, buffer, block_size);
    for (int ind=0; ind<ind_size;ind++) {
      recursive_block_delete(fs, buffer[ind], level-1);
    }
  }
  ext2_free_block(fs, block_address);
}

void ext2_free_inode_blocks(superblock_t* fs, int inode){
  ext2_inode_t data = ext2_get_inode_descriptor(fs, inode);
  for (int i=0;i<12;i++) {
    recursive_block_delete(fs, data.direct_block_ptr[i], 0);
    data.direct_block_ptr[i] = 0;
  }
  recursive_block_delete(fs, data.singly_indirect_block_ptr, 1);
  data.singly_indirect_block_ptr = 0;
  recursive_block_delete(fs, data.doubly_indirect_block_ptr, 2);
  data.doubly_indirect_block_ptr = 0;
  recursive_block_delete(fs, data.triply_indirect_block_ptr, 3);
  data.triply_indirect_block_ptr = 0;

  data.last_access_time = timer_get_posix_time();
  data.last_modification_time = timer_get_posix_time();
  data.size = 0;

  ext2_update_inode_data(fs, inode, data);
}


void ext2_update_inode_data(superblock_t* fs, int inode, ext2_inode_t data) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  ext2_extended_superblock_t* esb = devices[fs->id].esb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int block_group = (inode-1) / sb->inodes_per_group;
  int offset = (inode-1) % sb->inodes_per_group;

  ext2_block_group_descriptor_t group_descriptor
                            = ext2_get_block_group_descriptor(fs, block_group);

//  kernel_printf("Inode %d update. Data is %d %d %d\n", inode, data.size, data.direct_block_ptr[0], data.direct_block_ptr[1]);

  disk->write(group_descriptor.inode_table_addr*block_size + offset*esb->inode_size,
              &data,
              sizeof(ext2_inode_t));
}

bool ext2_register_inode(superblock_t* fs, int inode) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int block_group = (inode-1) / sb->inodes_per_group;
  int offset = (inode-1) % sb->inodes_per_group;

  ext2_block_group_descriptor_t group_descriptor
                            = ext2_get_block_group_descriptor(fs, block_group);

  uint8_t byte;
  disk->read(group_descriptor.inode_usage_bitmap_addr*block_size + offset/8,
             &byte,
             1);

  if (byte & (1 << (offset%8))) {
    kernel_printf("[ERROR][EXT2] Inode %d is already taken.", inode);
    return false;
  }
  group_descriptor.unallocated_inode_count--;
  ext2_set_block_group_descriptor(fs, group_descriptor, block_group);
  // TODO: Setup a mutex here..
  byte |= (1 << (offset%8));
  disk->write(group_descriptor.inode_usage_bitmap_addr*block_size + offset/8,
              &byte,
              1);

  // Now our inode is reserved
  return true;
}

bool ext2_register_block(superblock_t* fs, int number) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int block_group = (number) / sb->blocks_per_group;
  int offset = (number) % sb->blocks_per_group;

  ext2_block_group_descriptor_t group_descriptor
                            = ext2_get_block_group_descriptor(fs, block_group);

  uint8_t byte;
  disk->read(group_descriptor.block_usage_bitmap_addr*block_size + offset/8,
             &byte,
             1);

  if (byte & (1 << (offset%8))) {
    kernel_printf("[ERROR][EXT2] Block %d is already taken.", number);
    return false;
  }
  group_descriptor.unallocated_block_count--;
  ext2_set_block_group_descriptor(fs, group_descriptor, block_group);
  // TODO: Setup a mutex here..
  byte |= (1 << (offset%8));
  disk->write(group_descriptor.block_usage_bitmap_addr*block_size + offset/8,
              &byte,
              1);
  return true;
}

bool ext2_free_inode(superblock_t* fs, int inode) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int block_group = (inode-1) / sb->inodes_per_group;
  int offset = (inode-1) % sb->inodes_per_group;

  ext2_block_group_descriptor_t group_descriptor
                            = ext2_get_block_group_descriptor(fs, block_group);

  uint8_t byte;
  disk->read(group_descriptor.inode_usage_bitmap_addr*block_size + offset/8,
             &byte,
             1);

  if (!(byte & (1 << (offset%8)))) {
    kernel_printf("[ERROR][EXT2] Inode %d is already free.", inode);
    return false;
  }
  // TODO: Setup a mutex here..
  byte &= ~(1 << (offset%8));
  disk->write(group_descriptor.inode_usage_bitmap_addr*block_size + offset/8,
              &byte,
              1);
  // Now our inode is freed
  return true;
}

bool ext2_free_block(superblock_t* fs, int number) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  int block_group = (number) / sb->blocks_per_group;
  int offset = (number) % sb->blocks_per_group;

  ext2_block_group_descriptor_t group_descriptor
                            = ext2_get_block_group_descriptor(fs, block_group);

  uint8_t byte;
  disk->read(group_descriptor.block_usage_bitmap_addr*block_size + offset/8,
             &byte,
             1);

  if (!(byte & (1 << (offset%8)))) {
    kernel_printf("[ERROR][EXT2] Block %d is already free.", number);
    return false;
  }
  // TODO: Setup a mutex here..
  byte &= ~(1 << (offset%8));
  disk->write(group_descriptor.block_usage_bitmap_addr*block_size + offset/8,
              &byte,
              1);
  return true;
}
