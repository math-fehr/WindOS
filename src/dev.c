#include "dev.h"

#include "stdlib.h"
#include "errno.h"


static inode_operations_t dev_inode_operations = {
  .read_dir = dev_lsdir,
  .read = dev_fread,
  .write = dev_fwrite,
  .mkdir = NULL,
  .rm = NULL,
  .mkfile = NULL,
  .ioctl = dev_ioctl,
};

superblock_t* dev_initialize(int id) {
    superblock_t* res = malloc(sizeof(superblock_t));
    res->id = id;
    res->root.st.st_ino    = DEV_ROOT;
    res->root.st.st_size   = 0;
    res->root.st.st_mode   = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXG;
    res->root.st.st_dev    = id;
	res->root.sb 		   = res;
    res->root.op           = &dev_inode_operations;
    return res;
}

vfs_dir_list_t* dev_append_elem(inode_t inode, char* name, vfs_dir_list_t* lst) {
    vfs_dir_list_t* res = malloc(sizeof(vfs_dir_list_t));
    res->name = malloc(strlen(name)+1);
    strcpy(res->name, name);
    res->inode = inode;
    res->next = lst;
    return res;
}

int dev_ioctl(inode_t from, int cmd, int arg) {
	if (from.st.st_ino == DEV_SERIAL) {
		if (cmd == 0) { // set mode
			serial_setmode(arg);
		}
	}

	return 0;
}

vfs_dir_list_t* dev_lsdir(inode_t from) {
    if (from.st.st_ino == DEV_ROOT) {
        vfs_dir_list_t* res = NULL;

        inode_t r;
        r.st.st_ino = DEV_NULL;
        r.st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
        r.st.st_size = 0;
        r.sb = from.sb;
        r.op = &dev_inode_operations;
        res = dev_append_elem(r, "null", res);

        r.st.st_ino = DEV_ZERO;
        res = dev_append_elem(r, "zero", res);


        r.st.st_ino = DEV_RANDOM;
        res = dev_append_elem(r, "random", res);

        r.st.st_ino = DEV_SERIAL;
        r.st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
        r.st.st_size = 0;
		r.st.st_blksize = 1024;
		r.st.st_nlink = 1;
        res = dev_append_elem(r, "serial", res);


        r.st.st_ino = DEV_ROOT;
        res = dev_append_elem(r, ".", res);

        r.st.st_ino = DEV_ROOT;
        res = dev_append_elem(r, "..", res);
        return res;
    } else {
		kernel_printf("This is illegal.\n");
	}
	errno = ENOENT;
    return NULL;
}

int dev_fread(inode_t from, char* buf, int size, int pos) {
    switch (from.st.st_ino) {
        case DEV_NULL:
            return 0;
        case DEV_ZERO:
            for (int i=0;i<size;i++) {
                buf[i] = 0;
            }
            return size;
        case DEV_RANDOM:
            //TODO: RNG
            return size;
        case DEV_SERIAL:

            return serial_readline(buf, size);
    }
    return -1;
}

int dev_fwrite(inode_t from, char* buf, int size, int pos) {
    switch (from.st.st_ino) {
        case DEV_SERIAL:
            for (int i=0;i<size;i++) {
                serial_putc(buf[i]);
            }
            return size;
        default:
            return -1;
    }
}
