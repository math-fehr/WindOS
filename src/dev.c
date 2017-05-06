#include "dev.h"

#include "stdlib.h"


static inode_operations_t dev_inode_operations = {
  .read_dir = dev_lsdir,
  .read = dev_fread,
  .write = dev_fwrite,
  .mkdir = NULL,
  .rm = NULL,
  .mkfile = NULL,
};


superblock_t* dev_initialize(int id) {
    superblock_t* res = malloc(sizeof(superblock_t));
    res->id = id;
    res->root = malloc(sizeof(inode_t));
    res->root->st.st_ino    = DEV_ROOT;
    res->root->st.st_size   = 0;
    res->root->st.st_mode   = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXG;
    res->root->st.st_dev    = id;
    res->root->op           = &dev_inode_operations;
    return res;
}

vfs_dir_list_t* dev_append_elem(inode_t* inode, char* name, vfs_dir_list_t* lst) {
    vfs_dir_list_t* res = malloc(sizeof(vfs_dir_list_t));
    res->name = malloc(strlen(name)+1);
    strcpy(res->name, name);
    res->inode = inode;
    res->next = lst;
    return res;
}

vfs_dir_list_t* dev_lsdir(inode_t* from) {
    if (from->st.st_ino == DEV_ROOT) {
        vfs_dir_list_t* res = NULL;

        inode_t* r = malloc(sizeof(inode_t));
        r->st.st_ino = DEV_NULL;
        r->st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
        r->st.st_size = 0;
        r->sb = from->sb;
        r->op = &dev_inode_operations;
        res = dev_append_elem(r, "null", res);

        r = malloc(sizeof(inode_t));
        r->st.st_ino = DEV_ZERO;
        r->st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
        r->st.st_size = 0;
        r->sb = from->sb;
        r->op = &dev_inode_operations;
        res = dev_append_elem(r, "zero", res);

        r = malloc(sizeof(inode_t));
        r->st.st_ino = DEV_RANDOM;
        r->st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
        r->st.st_size = 0;
        r->sb = from->sb;
        r->op = &dev_inode_operations;
        res = dev_append_elem(r, "random", res);
        r = malloc(sizeof(inode_t));

        r->st.st_ino = DEV_SERIAL;
        r->st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
        r->st.st_size = 0;
		r->st.st_blksize = 1024;
		r->st.st_nlink = 1;
        r->sb = from->sb;
        r->op = &dev_inode_operations;
        res = dev_append_elem(r, "serial", res);
        return res;
    }
    return 0;
}

int dev_fread(inode_t* from, char* buf, int size, int pos) {
    switch (from->st.st_ino) {
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
        default:
            //TODO: Throw error
            return 0;
    }
    return 0;
}

int dev_fwrite(inode_t* from, char* buf, int size, int pos) {
    switch (from->st.st_ino) {
        case DEV_SERIAL:
            for (int i=0;i<size;i++) {
                serial_putc(buf[i]);
            }
            return size;
        default:
            return 0;
    }
}
