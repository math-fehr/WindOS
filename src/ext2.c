#include "ext2.h"
#include <stdlib.h>
#include <string.h>

ext2_device_t devices[10];
int i = 0;

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
  result->root = 2;
  i++;
  return result;
}

ext2_inode_t ext2_get_inode_descriptor(superblock_t* fs, int inode) {
//  kernel_info("ext2.c", "Info for inode");
//  print_hex(inode, 1);

  ext2_superblock_t* sb = devices[fs->id].sb;
  ext2_extended_superblock_t* esb = devices[fs->id].esb;
  storage_driver* disk = devices[fs->id].disk;

  int block_group = (inode-1) / sb->inodes_per_group;

  ext2_block_group_descriptor_t group_descriptor;
  int block_size = 1024 << sb->log_block_size;
  int bgdt = 2048;
  if (sb->log_block_size > 0) {
    bgdt = block_size;
  }
  int address = bgdt + 32*block_group;
  //kernel_info("ext2.c", "Group descriptor address: ");
//  print_hex(address, 4);
  disk->read(address, &group_descriptor, sizeof(group_descriptor));

  int index = (inode-1) % sb->inodes_per_group;

  ext2_inode_t inode_descriptor;
  address = group_descriptor.inode_table_addr * block_size + index * esb->inode_size;
  disk->read(address, &inode_descriptor, sizeof(inode_descriptor));
  return inode_descriptor;
}

// reads from a block: offset < block_size and offset+size <= block_size
void ext2_inode_read_block(superblock_t* fs, ext2_inode_t inode, char* buffer, int block, int offset, int size) {
  kernel_printf("[INFO][EXT2] Reading block %d for inode from %d to %d\n", block, offset, offset+size);
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

  if (last_block >= (12 + 256 + 256*256)) {
    kernel_printf("[ERROR][EXT2] Triply-indirect block not supported. \n");
    return 0;
  }

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
dir_list_t* ext2_lsdir(superblock_t* fs, int inode) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  //ext2_extended_superblock_t* esb = devices[fs->id].esb;
  storage_driver* disk = devices[fs->id].disk;

  dir_list_t* dir_list = 0;
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
          dir_list_t* entry = malloc(sizeof(dir_list_t));
          entry->next = dir_list;
          entry->name = name;
          entry->val  = l_inode;
          if (sb->major_version < 1) {
            ext2_inode_t r = ext2_get_inode_descriptor(fs, l_inode);
            switch (r.type_permissions & 0xF000) {
              case EXT2_INODE_FIFO:
                entry->attr = EXT2_ENTRY_FIFO;
                break;
              case EXT2_INODE_CHAR_DEVICE:
                entry->attr = EXT2_ENTRY_CHAR_DEVICE;
                break;
              case EXT2_INODE_DIRECTORY:
                entry->attr = EXT2_ENTRY_DIRECTORY;
                break;
              case EXT2_INODE_BLOCK_DEVICE:
                entry->attr = EXT2_ENTRY_BLOCK_DEVICE;
                break;
              case EXT2_INODE_FILE:
                entry->attr = EXT2_ENTRY_FILE;
                break;
              case EXT2_INODE_SYMLINK:
                entry->attr = EXT2_ENTRY_SYMLINK;
                break;
              case EXT2_INODE_SOCKET:
                entry->attr = EXT2_ENTRY_SOCKET;
                break;
            }
          } else {
            entry->attr = data[explorer+7];
          }
          dir_list = entry;

          explorer += entry_size;
        }
      }
    }
  }
  free(data);
  return dir_list;
}
