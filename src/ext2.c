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
        int l_inode;
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
