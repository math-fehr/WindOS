#include "fat.h"
#include "libc/string.h"

storage_driver* disk;
fat_bpb data_1;

fat_bs data_2;
fat32_bs data_2_fat32;

int fat_version;

int root_dir_sectors;
int FATsz;
int tot_sec;

int first_data_sector;


bool fat_init_fs(storage_driver* disk_) {
  disk = disk_;

  uint32_t signature=0;
  disk->read(510, &signature, 2 );

  if (signature != 0xAA55) {
    kernel_error("fat.c", "Wrong signature code.");
    print_hex(signature, 2);
    return false;
  }

  disk->read(0, &data_1, sizeof(fat_bpb));
  print_hex(data_1.sectors_per_cluster,2);
  print_hex(data_1.reserved_sectors_cnt,2);
  print_hex(data_1.bytes_per_sectors,2);

  disk->read(sizeof(fat_bpb), &data_2_fat32, sizeof(fat32_bs));

  root_dir_sectors = ((data_1.root_ent_cnt * 32) + (data_1.bytes_per_sectors - 1)) / data_1.bytes_per_sectors;

  if (data_1.FATsz16 != 0) {
    FATsz = data_1.FATsz16;
  } else {
    FATsz = data_2_fat32.FATsz32;
  }

  if (data_1.tot_sec_16 != 0) {
    tot_sec = data_1.tot_sec_16;
  } else {
    tot_sec = data_1.tot_sec_32;
  }
  print_hex(tot_sec,2);
  print_hex(FATsz,2);

  int data_sec = tot_sec - (data_1.reserved_sectors_cnt + (data_1.num_FATs * FATsz) + root_dir_sectors);
  int count_of_clusters = data_sec / data_1.sectors_per_cluster;

  if (count_of_clusters < 4085) {
    fat_version = 12;
    kernel_info("fat.c","FAT12 volume identified.");
    disk->read(sizeof(fat_bpb), &data_2, sizeof(fat_bs));

  } else if (count_of_clusters < 65525) {
    fat_version = 16;
    kernel_info("fat.c","FAT16 volume identified.");
    disk->read(sizeof(fat_bpb), &data_2, sizeof(fat_bs));

  } else {
    fat_version = 32;
    kernel_info("fat.c","FAT32 volume identified.");
    disk->read(64, &data_2, sizeof(fat_bs));
  }


  first_data_sector = data_1.reserved_sectors_cnt + (data_1.num_FATs * FATsz) + root_dir_sectors;


  char vol_label[12];
  memcpy(vol_label, &data_2.vol_label, 11);
  vol_label[11] = 0;
  kernel_info("fat.c", "FAT initialized, volume label is ");
  kernel_info("fat.c", vol_label);
  return true;
}

int first_sector_of_cluster(int N) {
  return ((N - 2)*data_1.sectors_per_cluster) + first_data_sector;
}

int entry_value_of_cluster(int N) {
  int result=0;
  if (fat_version == 12) {
    int fat_offset = N + (N / 2);
    /*
    int fat_sec_num = data_1->reserved_sectors_cnt + (fat_offset / data_1->bytes_per_sectors);
    int fat_ent_offset = fat_offset % data_1->bytes_per_sectors;
*/
    disk->read(
      data_1.reserved_sectors_cnt*data_1.bytes_per_sectors + fat_offset,
      &result,
      2
    ); // read 16 bits but we only need 12.
    if (N & 1) { // N is odd: we only want the 12 higher bits.
      return result >> 4;
    } else { // N is even: we only want the 12 lower bits.
      return (result & 0x0FFF);
    }
  } else {
    int fat_offset;
    if (fat_version == 16) {
      fat_offset = N*2;
      disk->read(
        data_1.reserved_sectors_cnt*data_1.bytes_per_sectors + fat_offset,
        &result,
        2
      );
    } else {
      fat_offset = N*4;
      disk->read(
        data_1.reserved_sectors_cnt*data_1.bytes_per_sectors + fat_offset,
        &result,
        4
      );
    }
    return result;
    /*
    int fat_sec_num = data_1->reserved_sectors_cnt + (fat_offset / data_1->bytes_per_sectors);
    int fat_ent_offset = fat_offset % data_1->bytes_per_sectors;
    */


  }
}

int fat_compute_root_directory() {
  if (fat_version == 32) {
    return first_sector_of_cluster(data_2_fat32.root_cluster);
  } else {
    return data_1.reserved_sectors_cnt + (data_1.num_FATs * FATsz);
  }
}

void fat_ls_root() {
  int pos_byte = fat_compute_root_directory()*
                  data_1.bytes_per_sectors;

  entry current_entry;
  char name_buffer[12];
  name_buffer[11] = 0; // Not sure if initialized.

  char long_name_buffer[32];

  kernel_info("fat.c", "Root listing..");
  int long_name_stack = 0;
  do {
    disk->read(pos_byte, &current_entry, 32);
    if (current_entry.attr == FAT_ATTR_LONG_NAME) { // long name
      long_name_stack += 1;
    } else {
      memcpy(name_buffer, &current_entry.name, 11);
      kernel_info("fat.c", name_buffer);
      if (long_name_stack > 0) {
        serial_write("=> ");

        for (int i=0;i<long_name_stack;i++) {
          disk->read(pos_byte-(i+1)*32, long_name_buffer, 32);
          bool stop=false;
          for (int k=0; k<5 && !stop; k++) {
            if (long_name_buffer[1+2*k] != 0) {
              serial_putc(long_name_buffer[1+2*k]);
            } else {
              stop = true;
            }
          }

          for (int k=0; k<6 && !stop; k++) {
            if (long_name_buffer[14+2*k] != 0) {
              serial_putc(long_name_buffer[14+2*k]);
            } else {
              stop = true;
            }
          }

          for (int k=0; k<2 && !stop;k++) {
            if (long_name_buffer[28+2*k] != 0) {
              serial_putc(long_name_buffer[28+2*k]);
            } else {
              stop = true;
            }
          }
        }
        serial_newline();
        long_name_stack = 0;
      }
    }
    pos_byte += 32;
  } while (current_entry.name[0] != 0);
}
