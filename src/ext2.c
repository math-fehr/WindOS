#include "ext2.h"
#include <stdlib.h>
#include <string.h>

ext2_device_t devices[10];
int i = 0;

superblock_t* ext2fs_initialize(storage_driver* disk) {
  ext2_superblock_t* sb = malloc(sizeof(ext2_superblock_t));
  disk->read(1024, sb, sizeof(ext2_superblock_t));

  if (sb->signature != 0xef53) {
    kernel_error("ext2.c", "Could not setup filesystem: wrong signature.");
    print_hex(sb->signature, 2);
    free(sb);
    return NULL;
  }

  ext2_extended_superblock_t* esb = malloc(sizeof(ext2_extended_superblock_t));
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

  superblock_t* result = malloc(sizeof(superblock_t));
  result->id = i;
  result->root = 2;
  i++;
  return result;
}

ext2_inode_t ext2_get_inode_descriptor(superblock_t* fs, int inode) {
  kernel_info("ext2.c", "Info for inode");
  print_hex(inode, 1);

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
  kernel_info("ext2.c", "Group descriptor address: ");
  print_hex(address, 4);
  disk->read(address, &group_descriptor, sizeof(group_descriptor));

  int index = (inode-1) % sb->inodes_per_group;

  ext2_inode_t inode_descriptor;
  address = group_descriptor.inode_table_addr * block_size + index * esb->inode_size;
  disk->read(address, &inode_descriptor, sizeof(inode_descriptor));
  return inode_descriptor;
}

void lsdir(superblock_t* fs, int inode) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  ext2_extended_superblock_t* esb = devices[fs->id].esb;
  storage_driver* disk = devices[fs->id].disk;

  ext2_inode_t result = ext2_get_inode_descriptor(fs, inode);
  int block_size = 1024 << sb->log_block_size;
  uint8_t* data = malloc(block_size);
  for (int i=0; i<12; i++) {
    if (result.direct_block_ptr[i] != 0) { // There is data in the block
      print_hex(result.direct_block_ptr[i]*block_size,4);
      disk->read(result.direct_block_ptr[i]*block_size, data, block_size);
      int explorer = 0;
      while (explorer < block_size) {
        int entry_size = data[explorer+4] + (data[explorer+5] << 8);
        int l_inode    = (uint32_t) (*data);
        if (l_inode == 0) {
          if (entry_size == 0) {
            explorer = block_size;
          } else {
            explorer += entry_size;
          }
        } else {
          uint8_t length = data[explorer+6];
          char* name = malloc(length+1);
          name[length] = 0;
          memcpy(name, &data[explorer+8], length);
          kernel_info("ext2.c", name);
          explorer += entry_size;
          free(name);
        }
      }
    }
  }
  kernel_info("ext2.c", "Finished.");
  free(data);
}
