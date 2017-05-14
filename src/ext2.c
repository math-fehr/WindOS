#include "ext2.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

ext2_device_t devices[10];
int i = 0;

static inode_operations_t ext2_inode_operations = {
  .read_dir = ext2_lsdir,
  .read = ext2_fread,
  .write = ext2_fwrite,
  .mkdir = ext2_mkdir,
  .rm = ext2_rm,
  .mkfile = ext2_mkfile,
  .resize = ext2_resize
};

/*
 * Gather data from superblock and do stuff.
 */
superblock_t* ext2fs_initialize(
		storage_driver* disk)
{
	if (disk == NULL) {
		errno = EFAULT;
		return NULL;
	}

	ext2_superblock_t* sb =
		(ext2_superblock_t*) malloc(sizeof(ext2_superblock_t));
  	disk->read(
		1024,
		sb,
		sizeof(ext2_superblock_t));

	if (sb->signature != 0xef53) {
	    kdebug(D_EXT2, 3, "Could not setup filesystem: wrong signature.\n \
	                       0xEF53 != %#04X\n", sb->signature);
	    free(sb);
		errno = EIO;
	    return NULL;
	}
	kdebug(D_EXT2, 1, "Signature found. Loading filesystem superblock.\n");
	ext2_extended_superblock_t* esb =
		(ext2_extended_superblock_t*)malloc(sizeof(ext2_extended_superblock_t));

	kdebug(D_EXT2,1,"Version is %d.%d\n", sb->major_version, sb->minor_version);
	if (sb->major_version >= 1) {
		disk->read(
			1024 + 84,
			esb,
			sizeof(ext2_extended_superblock_t));
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

	ext2_inode_t desc = ext2_get_inode_descriptor(result, 2);

	result->root.st = ext2_inode_to_stat(result, desc, 2);
	result->root.sb = result;
	result->root.op = &ext2_inode_operations;
	i++;
	return result;
}

/**
 * Takes inode data and put it into a stat format.
 */
struct stat ext2_inode_to_stat(
		superblock_t* sb, ext2_inode_t data, int number)
{
	struct stat res;
	if (sb == NULL) {
		errno = EFAULT;
		return res;
	}

    res.st_atime        = data.last_access_time;
    res.st_blksize      = 1024 << devices[sb->id].sb->log_block_size;
    res.st_blocks       = (data.size+511) / 512;
    res.st_ctime        = data.creation_time;
    res.st_dev          = sb->id;
    res.st_gid          = data.group_id;
    res.st_ino          = number;
    res.st_mode         = data.type_permissions;
    res.st_mtime        = data.last_modification_time;
    res.st_nlink        = data.hard_links;
    res.st_rdev         = 0;
    res.st_size         = data.size;
    res.st_uid          = data.user_id;
    return res;
}

ext2_block_group_descriptor_t ext2_get_block_group_descriptor(
		superblock_t* fs, int bg)
{
	ext2_superblock_t* sb = devices[fs->id].sb;
	storage_driver* disk = devices[fs->id].disk;


	ext2_block_group_descriptor_t group_descriptor;
	int block_size = 1024 << sb->log_block_size;
	int bgdt = 2048;
	if (sb->log_block_size > 0) {
		bgdt = block_size;
	}
	int address = bgdt + 32*bg;

	disk->read(
		address,
		&group_descriptor,
		sizeof(ext2_block_group_descriptor_t));

	return group_descriptor;
}

void ext2_set_block_group_descriptor(
		superblock_t* fs, ext2_block_group_descriptor_t data, int bg)
{
	ext2_superblock_t* sb = devices[fs->id].sb;
	storage_driver* disk = devices[fs->id].disk;

	int block_size = 1024 << sb->log_block_size;
	int bgdt = 2048;
	if (sb->log_block_size > 0) {
		bgdt = block_size;
	}
	int address = bgdt + 32*bg;
	disk->write(
		address,
		&data,
		sizeof(ext2_block_group_descriptor_t));
}

ext2_inode_t ext2_get_inode_descriptor(
		superblock_t* fs, int inode)
{

	ext2_superblock_t* sb = devices[fs->id].sb;
	ext2_extended_superblock_t* esb = devices[fs->id].esb;
	storage_driver* disk = devices[fs->id].disk;

	int block_group = (inode-1) / sb->inodes_per_group;
	int block_size = 1024 << sb->log_block_size;

	ext2_block_group_descriptor_t group_descriptor =
	ext2_get_block_group_descriptor(fs, block_group);

	int index = (inode-1) % sb->inodes_per_group;

	ext2_inode_t inode_descriptor;
	int address = group_descriptor.inode_table_addr * block_size
					+ index * esb->inode_size;
	disk->read(
		address,
		&inode_descriptor,
		sizeof(inode_descriptor));
	return inode_descriptor;
}



uintptr_t ext2_get_block_address(
		superblock_t* fs, ext2_inode_t inode, int block)
{
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
		disk->read(
			s_ind_block*block_size + 4*block_1,
			&base_address,
			4);
	} else if (block < 12+ind_size+ind_size*ind_size) {// doubly indirect block
		block = block - 12 - ind_size;
		int block_1 = block / ind_size;
		int block_2 = block % ind_size;
		// Address of the first block;
		uintptr_t d_ind_block = (uintptr_t) inode.doubly_indirect_block_ptr;
		uintptr_t s_ind_block = 0;
		disk->read(
			d_ind_block*block_size + 4*block_1,
			&s_ind_block,
			4);
		disk->read(
			s_ind_block*block_size + 4*block_2,
			&base_address,
			4);
	} else { // triply indirect block
		block = block - 12 - ind_size - ind_size*ind_size;
		int block_1 = block / (ind_size*ind_size);
		int block_2 = (block % (ind_size*ind_size)) / ind_size;
		int block_3 = block % ind_size;
		uintptr_t t_ind_block = (uintptr_t) inode.triply_indirect_block_ptr;
		uintptr_t d_ind_block = 0;
		uintptr_t s_ind_block = 0;
		// From the first block, reads the address of the second block.
		disk->read(
			t_ind_block*block_size + 4*block_1,
			&d_ind_block,
			4);
		// From the second block, reads the address of the third block.
		disk->read(
			d_ind_block*block_size + 4*block_2,
			&s_ind_block,
			4);
		// From the third block, reads the address of the data block.
		disk->read
		(s_ind_block*block_size + 4*block_3,
			&base_address,
			4);
	}
	return base_address;
}

// reads from a block: offset < block_size and offset+size <= block_size
void ext2_inode_read_block(
		superblock_t* fs, ext2_inode_t inode,
		char* buffer, int block, int offset, int size)
{
	ext2_superblock_t* sb = devices[fs->id].sb;
	storage_driver* disk = devices[fs->id].disk;

	int block_size = 1024 << sb->log_block_size;
	uintptr_t base_address = ext2_get_block_address(fs, inode, block);
	disk->read(
		base_address*block_size+offset,
		buffer,
		size);
}


// TODO: update last access time
int ext2_fread(
		inode_t vfs_inode, char* buffer, int size, int position)
{
	superblock_t* fs = vfs_inode.sb;
	int inode = vfs_inode.st.st_ino;

	//kernel_printf("[INFO][EXT2] Reading file inode %d\n", inode);
	ext2_superblock_t* sb = devices[fs->id].sb;

	ext2_inode_t info = ext2_get_inode_descriptor(fs, inode);
	if (!(info.type_permissions & EXT2_INODE_FILE)) {
		kdebug(D_EXT2, 2, "Inode %d is not a file.\n", inode);
		return 0;
	}

	if ((uint32_t)(position + size) > info.size) {
		size = info.size - position;
	}

	int block_size = 1024 << sb->log_block_size;
	int first_block = position / block_size;
	int last_block = (position+size-1) / block_size;

	int offset = position % block_size;
	kdebug(D_EXT2, 2,
		"Reading file: size %#010x position %#010x to buffer %#010x\n",
		size, position, buffer);

	if (offset+size <= block_size) { // data on single block
		ext2_inode_read_block(fs, info,
			buffer, first_block, offset, size);
	} else { // data on several blocks
		kdebug(D_EXT2, 0, "From block %d to %d\n", first_block, last_block);
		ext2_inode_read_block(fs, info,
			buffer, first_block, offset, block_size-offset);
		int buffer_position = block_size - offset;
		for (int block=first_block+1; block < last_block; block++) {
	    	kdebug(D_EXT2, 0, "B: %d\n", block);
	  		ext2_inode_read_block(fs, info,
				buffer+buffer_position, block, 0, block_size);
	  		buffer_position += block_size;
		}
		ext2_inode_read_block(fs, info,
			buffer+buffer_position, last_block, 0, size-buffer_position);
	}
	return size;
}


 // |inode(4)|size(2)|length(1)|type(1)|name(N)
 // TODO: update last modification time
void ext2_add_dir_entry(
		superblock_t* fs, int dir, int dest, char* name)
{
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
	memcpy(entry+8, name, strlen(name)); // a strcpy writes a zero at the end.
	if (new_entry_size > block_size-last_entry_size-pos) { // create a new block
		uint16_t w_size = block_size;
		memcpy(entry+4, &w_size, 2);

		ext2_append_file(fs, dir, entry, new_entry_size);
		int n_size = data.size+block_size;
		data = ext2_get_inode_descriptor(fs, dir);
		data.size = n_size;
		ext2_update_inode_data(fs, dir, data);
	} else { // append and update previous entry.
		last_entry_size = 4*((last_entry_size + 3)/4); // 4-byte align.
		ext2_replace_file(fs, dir,
			(char*)&last_entry_size, 2, 4+pos+last_block*block_size);

		uint16_t w_size = block_size-last_entry_size-pos;
		memcpy(entry+4, &w_size, 2);

		ext2_replace_file(fs, dir,
			entry, new_entry_size, last_entry_size+pos+last_block*block_size);
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
	ext2_add_dir_entry(inode.sb, new_dir, inode.st.st_ino, "..");

	ext2_add_dir_entry(inode.sb, inode.st.st_ino, new_dir, name);
	return 1;
}


int ext2_resize(inode_t inode, int new_size) {
	superblock_t* fs = inode.sb;
	ext2_superblock_t* sb = devices[fs->id].sb;
	storage_driver* disk = devices[fs->id].disk;
	int block_size = 1024 << sb->log_block_size;

	if (new_size < 0) {
		errno = EINVAL;
		return -1;
	}



	ext2_inode_t inode_desc = ext2_get_inode_descriptor(fs, inode.st.st_ino);
	int old_size = inode_desc.size;
	if (old_size > 12*block_size) {
		kernel_printf("Critical error: base size is too large. Not implemented.");
		return -1;
	}

	if (new_size > old_size) {
		char* buf = malloc(new_size-old_size);
		if (buf != 0) {
			ext2_fwrite(inode, buf, new_size-old_size, old_size);
			return new_size;
		} else {
			return -1;
		}
	} else if (new_size < old_size) {
		int new_block_base = (new_size+block_size-1)/block_size;
		for (int i=new_block_base+1;i<(old_size+block_size-1)/block_size;i++) {
			recursive_block_delete(fs, inode_desc.direct_block_ptr[i], 0);
			inode_desc.direct_block_ptr[i] = 0;
		}
		inode_desc.size = new_size;
	}
	ext2_update_inode_data(fs, inode.st.st_ino, inode_desc);
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

	ext2_inode_t container_desc =ext2_get_inode_descriptor(fs, inode.st.st_ino);
	char* data = malloc(block_size);

	bool found = false;
	bool deleted = false;

	while ((!found) &&
	(addr = ext2_get_block_address(fs, container_desc, current_block)) != 0)
	{
		disk->read(addr*block_size, data, block_size);
		int explorer = 0;
		while (explorer < block_size)
		{
			int entry_size = data[explorer+4] + (data[explorer+5] << 8);
			int name_size = data[explorer+6];
			if (name_size == n && (0 == strncmp(&data[explorer+8], name, n)))
			{
				found = true;
				int deleted_inode=0;
				memcpy(&deleted_inode, &data[explorer], 4);
				ext2_inode_t desc =ext2_get_inode_descriptor(fs, deleted_inode);
				bool ok_to_delete = true;
				if (desc.type_permissions & EXT2_INODE_DIRECTORY)
				{
					inode_t t;
					t.st.st_ino = deleted_inode;
					t.sb = fs;
					vfs_dir_list_t* lst = ext2_lsdir(t);
					int cnt = 0;
					while (lst != NULL)
					{
            			if(strcmp(".",lst->name) != 0
						&& strcmp("..",lst->name) != 0)
						{
              				cnt++;
            			}
            			lst = lst->next;
          			}
					if (cnt > 0)
					{
						kdebug(D_EXT2, 2, "Directory isn't empty. (%d)\n",cnt);
						ok_to_delete = false;
						errno = ENOTEMPTY;
					}
        		}
				if (ok_to_delete)
				{ // mark the inode as deleted.
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
		return 0;
	} else if (errno == 0){
		errno = ENOENT;
	}

  	return -1;
}

ext2_inode_t ext2_create_inode(int perm) {
	ext2_inode_t data;
	data.creation_time = timer_get_posix_time();
	data.last_access_time = timer_get_posix_time();
	data.group_id = 0;
	data.size = 0;
	data.user_id = 0;
	data.type_permissions = EXT2_INODE_FILE | perm;
	data.last_modification_time = timer_get_posix_time();
	data.hard_links = 1;
	for (int i=0;i<12;i++) {
		data.direct_block_ptr[i] = 0;
	}
	data.singly_indirect_block_ptr = 0;
	data.doubly_indirect_block_ptr = 0;
	data.triply_indirect_block_ptr = 0;
	data.flags = 0;
	return data;
}

int ext2_mkfile (inode_t inode, char* name, int perm) {
	int new_file = ext2_get_free_inode(inode.sb);
	ext2_register_inode(inode.sb, new_file);
	ext2_inode_t data = ext2_create_inode(perm);
	ext2_update_inode_data(inode.sb, new_file, data);

	ext2_add_dir_entry(inode.sb, inode.st.st_ino, new_file, name);
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

  int inode = inode_p.st.st_ino;

  vfs_dir_list_t* dir_list = 0;
  ext2_inode_t result = ext2_get_inode_descriptor(fs, inode);
  if (!(result.type_permissions & EXT2_INODE_DIRECTORY)) {
    kdebug(D_EXT2, 2, "Inode %d is not a directory.\n", inode);
    return 0;
  }

  int block_size = 1024 << sb->log_block_size;
  int n_blocks = result.size / block_size;
  uint8_t* data =  (uint8_t*) malloc(block_size);
  kdebug(D_EXT2, 1, "Inode %d, %d blocks\n", inode, n_blocks);
  for (int i=0; i<n_blocks; i++) {
    uintptr_t addr = ext2_get_block_address(fs, result, i);
    if (addr != 0) { // There is data in the block
      disk->read(addr*block_size, data, block_size);
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
          memcpy(name, &data[explorer+8], 4*((length+3)/4)); //4-byte align TODO: check if useful
          vfs_dir_list_t* entry = malloc(sizeof(vfs_dir_list_t));
          entry->next = dir_list;
          entry->name = name;

          entry->inode.sb = fs;
          entry->inode.op = &ext2_inode_operations;
          ext2_inode_t r = ext2_get_inode_descriptor(fs, l_inode);
          entry->inode.st = ext2_inode_to_stat(fs, r, l_inode);

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

  kdebug(D_EXT2, 0, "I: %d, %d\n", sb->total_inode_count, sb->inodes_per_group);

  for (unsigned i=0; i<sb->total_inode_count/sb->inodes_per_group;i++) {
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
      int free_inode = 1 + found + sb->inodes_per_group*i;
      kdebug(D_EXT2, 1, "ext2_get_free_inode: here you go: %d\n", free_inode);
      return free_inode;
    }
  }
  kdebug(D_EXT2, 2, "ext2_get_free_inode: error. No available inode.\n");
  return 0;
}

// Returns the first available block
int ext2_get_free_block(superblock_t* fs) {
  ext2_superblock_t* sb = devices[fs->id].sb;
  storage_driver* disk = devices[fs->id].disk;

  ext2_block_group_descriptor_t group_descriptor;
  int block_size = 1024 << sb->log_block_size;

  kdebug(D_EXT2, 0, "B: %d, %d\n", sb->total_block_count, sb->blocks_per_group);
  for (unsigned i=0; i<sb->total_block_count / sb->blocks_per_group;i++) {
    group_descriptor = ext2_get_block_group_descriptor(fs, i);
    kdebug(D_EXT2, 0, "%d, %d, %d\n", group_descriptor.unallocated_block_count,
    group_descriptor.unallocated_inode_count, group_descriptor.directory_count);
    if (group_descriptor.unallocated_block_count > 0) {
      uint8_t* bitmap = malloc(block_size);
      disk->read(group_descriptor.block_usage_bitmap_addr*block_size, bitmap, block_size);
      int found = -1;
      for (int b=0;b<block_size && found == -1;b++) {
        if (bitmap[b] != 0xFF) {
          kdebug(D_EXT2, 0, "%#02x\n", bitmap[b]);
          for (int bit=0;bit<8;bit++) {
            if (!(bitmap[b] & (1 << bit))) { // bit is 0
              found = b*8 + bit;
              kdebug(D_EXT2, 1, "B: %d, %d\n", b, bit);
            }
          }
        }
      }
      free(bitmap);
      kdebug(D_EXT2, 3, "ext2_get_free_block: here you go: %d\n", 1+found + sb->blocks_per_group*i);
      return 1+found + sb->blocks_per_group*i;
    }
  }
  kdebug(D_EXT2, 10, "ext2_get_free_block: error. No available block.");
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


void ext2_set_inode_block_address(
		superblock_t* fs, int inode, int b, int b_address)
{
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
    recursive_setup_block(fs, data.singly_indirect_block_ptr,
		b-12, b_address, 0);
  } else if(b < 12 + ind_size + ind_size * ind_size) {
    if (data.doubly_indirect_block_ptr == 0) {
      data.doubly_indirect_block_ptr = ext2_get_free_block(fs);
      ext2_register_block(fs, data.doubly_indirect_block_ptr);
    }
    recursive_setup_block(fs, data.doubly_indirect_block_ptr,
		b-12-ind_size, b_address, 1);
  } else {
    if (data.triply_indirect_block_ptr == 0) {
      data.triply_indirect_block_ptr = ext2_get_free_block(fs);
      ext2_register_block(fs, data.triply_indirect_block_ptr);
    }
    recursive_setup_block(fs, data.triply_indirect_block_ptr,
		b-12-ind_size-ind_size*ind_size, b_address, 2);
  }

  ext2_update_inode_data(fs, inode, data);
}


int ext2_fwrite(inode_t inode, char* buf, int len, int ofs) {
  superblock_t* fs = inode.sb;

  ext2_inode_t data = ext2_get_inode_descriptor(fs, inode.st.st_ino);

  int to_append = max(len+ofs - data.size, 0);
  int to_replace = len - to_append;

  ext2_replace_file(fs, inode.st.st_ino, buf, to_replace, ofs);
  ext2_append_file(fs, inode.st.st_ino, buf+to_replace, to_append);
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
  kernel_printf("%d %d \n", n_blocks_to_create, last_block);

  if (data.size == 0)
    last_block = -1;

  if (last_block != -1) {
    uintptr_t last_block_address = ext2_get_block_address(fs, data, last_block);
    disk->write(last_block_address*block_size, buffer, fill_blk);
  }

  kernel_printf("oui");

  int buf_pos = fill_blk; // position in the buffer.

  for (int b = last_block+1; b < last_block+1+n_blocks_to_create; b++) {
    int b_address = ext2_get_free_block(fs);
	if (b_address != -1) {
		ext2_register_block(fs, b_address);
	   	ext2_set_inode_block_address(fs, inode, b, b_address);
	   	disk->write(b_address*block_size, buffer + buf_pos, min(size-buf_pos, block_size));
	   	buf_pos += block_size;
	} else {
		break;
	}
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
    kdebug(D_EXT2, 3, "Inode %d is already taken.", inode);
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
  int block_group = (number-1) / sb->blocks_per_group;
  int offset = (number-1) % sb->blocks_per_group;

  ext2_block_group_descriptor_t group_descriptor
                            = ext2_get_block_group_descriptor(fs, block_group);

  uint8_t byte;
  disk->read(group_descriptor.block_usage_bitmap_addr*block_size + offset/8,
             &byte,
             1);

  if (byte & (1 << (offset%8))) {
    kdebug(D_EXT2, 10, "Block %d is already taken.", number);
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
    kdebug(D_EXT2, 2, "Inode %d is already free.", inode);
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
    kdebug(D_EXT2, 2, "Block %d is already free.", number);
    return false;
  }
  // TODO: Setup a mutex here..
  byte &= ~(1 << (offset%8));
  disk->write(group_descriptor.block_usage_bitmap_addr*block_size + offset/8,
              &byte,
              1);
  return true;
}
