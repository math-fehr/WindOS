#ifndef FAT_H
#define FAT_H

#include "storage_driver.h"
#include "debug.h"
#include "vfs.h" 

// https://staff.washington.edu/dittrich/misc/fatgen103.pdf

/**
 * Theses functions are used to manipulate the fat filesystem
 */

#pragma pack(push, 1)

typedef struct {
  uint8_t jmp_boot[3];
  uint8_t OEM_name[8];
  uint16_t bytes_per_sectors;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors_cnt;
  uint8_t num_FATs;
  uint16_t root_ent_cnt;
  uint16_t tot_sec_16;
  uint8_t media;
  uint16_t FATsz16;
  uint16_t sec_per_trk;
  uint16_t num_heads;
  uint32_t hidden_sec;
  uint32_t tot_sec_32;
} fat_bpb;

typedef struct {
  uint8_t drv_num;
  uint8_t reserved1;
  uint8_t bootsig;
  uint32_t vol_id;
  uint8_t vol_label[11];
  uint8_t fs_type[8];
} fat_bs;


typedef struct {
  uint32_t FATsz32;
  uint16_t ext_flags;
  uint16_t FS_version;
  uint16_t root_cluster;
  uint16_t FS_info;
  uint16_t bk_boot_sec;
} fat32_bs;


typedef struct {
  uint8_t name[11];
  uint8_t attr;
  uint8_t NT_res;
  uint8_t crt_time_tenth;
  uint16_t crt_time;
  uint16_t crt_date;
  uint16_t last_access_date;
  uint16_t first_cluster_number_LO;
  uint16_t wrt_time;
  uint16_t wrt_date;
  uint16_t first_cluster_number_HI;
  uint32_t file_size;
} fat_entry_t;

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LONG_NAME (0x01 | 0x02 | 0x04 | 0x08)

#pragma pack(pop)

typedef struct {
  storage_driver* disk;
  fat_bpb data_1;
  fat_bs data_2;
  fat32_bs data_2_fat32;
  int fat_version;
  int root_dir_sectors;
  int FATsz;
  int tot_sec;
  int first_data_sector;
} fatinfo_t;

superblock_t* fat_init_fs(storage_driver* disk);
int fat_compute_root_directory();
void fat_ls_root();

#endif //FAT_H
