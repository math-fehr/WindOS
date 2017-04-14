#ifndef EXT2_H
#define EXT2_H

#include "storage_driver.h"
#include "debug.h"
#include "vfs.h"
#include "timer.h"

//http://wiki.osdev.org/Ext2

#pragma pack(push, 1)
 
typedef struct {
  uint32_t total_inode_count;
  uint32_t total_block_count;
  uint32_t reserved_block_count;
  uint32_t unallocated_block_count;
  uint32_t unallocated_inode_count;
  uint32_t block_number_sb;
  uint32_t log_block_size; // log(blocksize) - 10
  uint32_t log_frag_size;
  uint32_t blocks_per_group;
  uint32_t fragments_per_group;
  uint32_t inodes_per_group;
  uint32_t last_mount_time; // posix time
  uint32_t last_writtent_time;
  uint16_t number_of_mnt_since_fsck;
  uint16_t number_of_mnt_allowed_before_fsck;
  uint16_t signature;
  uint16_t fs_state;
  uint16_t error_handling;
  uint16_t minor_version;
  uint32_t posix_fsck;
  uint32_t interval_fsck;
  uint32_t OSID;
  uint32_t major_version;
} ext2_superblock_t;

typedef struct {
  uint32_t first_non_reserved_inode; // Major < 1 => 11
  uint32_t inode_size; // Major < 1 => 128
  uint16_t block_grp;
  uint32_t optional_features;
  uint32_t required_features;
  uint32_t readonly_features;
} ext2_extended_superblock_t;

/*
From the Superblock, extract the size of each block, the total number of inodes, the total number of blocks, the number of blocks per block group, and the number of inodes in each block group. From this information we can infer the number of block groups there are by:

    Rounding up the total number of blocks divided by the number of blocks per block group
    Rounding up the total number of inodes divided by the number of inodes per block group
    Both (and check them against each other)
    */

typedef struct {
  uint32_t block_usage_bitmap_addr;
  uint32_t inode_usage_bitmap_addr;
  uint32_t inode_table_addr;
  uint16_t unallocated_block_count;
  uint16_t unallocated_inode_count;
  uint16_t directory_count;
} ext2_block_group_descriptor_t;


typedef struct {
  uint16_t type_permissions;
  uint16_t user_id;
  uint32_t size;
  uint32_t last_access_time;
  uint32_t creation_time;
  uint32_t last_modification_time;
  uint32_t deletion_time;
  uint16_t group_id;
  uint16_t hard_links;
  uint32_t disk_sector_count;
  uint32_t flags;
  uint32_t OS_specific;
  uint32_t direct_block_ptr[12];
  uint32_t singly_indirect_block_ptr;
  uint32_t doubly_indirect_block_ptr;
  uint32_t triply_indirect_block_ptr;
  uint32_t wat[3];
  uint32_t fragment_block_addr;
  uint32_t OS_specific2;
} ext2_inode_t;

#define EXT2_INODE_FIFO         0x1000
#define EXT2_INODE_CHAR_DEVICE  0x2000
#define EXT2_INODE_DIRECTORY    0x4000
#define EXT2_INODE_BLOCK_DEVICE 0x6000
#define EXT2_INODE_FILE         0x8000
#define EXT2_INODE_SYMLINK      0xA000
#define EXT2_INODE_SOCKET       0xC000

typedef struct {
  uint32_t inode;
  uint16_t size;
  uint8_t name_length_lsb;
  uint8_t type_indicator;
  char* name;
} ext2_dir_entry;

#define EXT2_ENTRY_UNK          0
#define EXT2_ENTRY_FILE         1
#define EXT2_ENTRY_DIRECTORY    2
#define EXT2_ENTRY_CHAR_DEVICE  3
#define EXT2_ENTRY_BLOCK_DEVICE 4
#define EXT2_ENTRY_FIFO         5
#define EXT2_ENTRY_SOCKET       6
#define EXT2_ENTRY_SYMLINK      7

#pragma pack(pop)

typedef struct {
  storage_driver* disk;
  ext2_extended_superblock_t* esb;
  ext2_superblock_t* sb;
} ext2_device_t;


superblock_t* ext2fs_initialize(storage_driver* disk);

// Interface functions with the VFS
int ext2_fwrite(inode_t inode, char* buf, int len, int ofs);
int ext2_fread(inode_t inode, char* buffer, int len, int ofs);
vfs_dir_list_t* ext2_lsdir(inode_t inode_p);


void ext2_replace_file(superblock_t* fs, int inode, char* buffer, int size, int ofs);
void ext2_append_file(superblock_t* fs, int inode, char* buffer, int size);

void ext2_inode_read_block(superblock_t* fs, ext2_inode_t inode, char* buffer, int block, int offset, int size);
ext2_inode_t ext2_get_inode_descriptor(superblock_t* fs, int inode);

int ext2_get_free_inode(superblock_t* fs);
int ext2_get_free_block(superblock_t* fs);
void ext2_update_inode_data(superblock_t* fs, int inode, ext2_inode_t data);
bool ext2_register_inode(superblock_t* fs, int inode);
bool ext2_register_block(superblock_t* fs, int number);
bool ext2_free_inode(superblock_t* fs, int inode);
bool ext2_free_block(superblock_t* fs, int number);

void ext2_free_inode_blocks(superblock_t* fs, int inode);

void ext2_set_block_group_descriptor(superblock_t* fs, ext2_block_group_descriptor_t data, int bg);

int ext2_mkdir (inode_t, char*, int);
int ext2_rm (inode_t, char*);
int ext2_mkfile (inode_t, char*, int);
#endif
