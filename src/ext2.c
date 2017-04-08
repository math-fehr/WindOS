#include "ext2.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

ext2_device_t devices[10];
int i = 0;

static inode_operations_t ext2_inode_operations = {
  .read_dir = ext2_lsdir
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
  disk->read(address, &group_descriptor, sizeof(group_descriptor));

  return group_descriptor;
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
  kernel_printf("[INFO][EXT2] Reading block %d for inode from %d to %d\n", block, offset, offset+size);
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  int block_size = 1024 << sb->log_block_size;
  uintptr_t base_address = ext2_get_block_address(fs, inode, block);
  disk->read(base_address*block_size+offset, buffer, size);
}



int ext2_fread(superblock_t* fs, int inode, char* buffer, int position, int size) {
  kernel_printf("[INFO][EXT2] Reading file inode %d\n", inode);
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
  return size;
}

/*
 * List the child elements of an inode.
 */
vfs_dir_list_t* ext2_lsdir(superblock_t* fs, inode_t inode_p) {
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
  int last_block = (data.size-1)/block_size;
  uintptr_t last_block_address = ext2_get_block_address(fs, data, last_block);
  disk->write(last_block_address*block_size, buffer, fill_blk);

  int buf_pos = fill_blk; // position in the buffer.

  for (int b = last_block+1; b < last_block+1+n_blocks_to_create; b++) {
    int b_address = ext2_get_free_block(fs);
    ext2_register_block(fs, b_address);
    ext2_set_inode_block_address(fs, inode, b, b_address);
    disk->write(b_address, buffer + buf_pos, min(size-buf_pos, block_size));
    buf_pos += block_size;
  }
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
