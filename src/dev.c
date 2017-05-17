#include "dev.h"

/** \file dev.c
  * \brief A mountable device pseudo-filesystem
  * These functions offers a filesystem interface to device features such as the
  * serial port.
  */

/**	\var static inode_operations_t dev_inode_operations
 *	Functions providing the inode interface with the VFS.
 */
static inode_operations_t dev_inode_operations = {
  .read_dir = dev_lsdir,
  .read = dev_fread,
  .write = dev_fwrite,
  .mkdir = NULL,
  .rm = NULL,
  .mkfile = NULL,
  .ioctl = dev_ioctl,
};

/**	\fn superblock_t* dev_initialize(int fd)
 *	\brief Builds a superblock, being the main interface object with the VFS.
 *	\param id The id of the newly created superblock.
 * 	\return A newly malloc'd superblock representing devices.
 */
superblock_t* dev_initialize (int id) {
    superblock_t* res = malloc(sizeof(superblock_t));
    res->id = id;
    res->root.st.st_ino    = DEV_ROOT;
    res->root.st.st_size   = 1024;
    res->root.st.st_mode   = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXG;
    res->root.st.st_dev    = id;
	res->root.sb 		   = res;
    res->root.op           = &dev_inode_operations;
    return res;
}

#include "framebuffer.h"
#include "fb.h"
extern frameBuffer* kernel_framebuffer;

/** \fn int dev_ioctl (inode_t from, int cmd, int arg)
 * 	\brief Modifies the attributes of a device.
 *	\param from The inode representing the device.
 * 	\param cmd The control command.
 *	\param arg The control command parameter.
 * 	\return 0 on success, -1 on fail.
 */
int dev_ioctl (inode_t from, int cmd, int arg) {
	if (from.st.st_ino == DEV_SERIAL) {
		if (cmd == 0) { // 0 is the set mode command.
			serial_setmode(arg);
			return 0;
		}
	} else if (from.st.st_ino == DEV_FB) {
		switch (cmd) {
			case FB_WIDTH:
				return kernel_framebuffer->width;
				break;
			case FB_HEIGHT:
				return kernel_framebuffer->height;
				break;
			case FB_OPEN:
				return fb_open(get_current_process_id());
				break;
			case FB_CLOSE:
				return fb_close(get_current_process_id());
				break;
			case FB_SHOW:
				return fb_show(get_current_process_id());
				break;
			case FB_FLUSH:
				return fb_flush(get_current_process_id());
				break;
			case FB_BUFFERED:
				return fb_buffered(get_current_process_id(),arg);
		}
	}
	return -1;
}

/**	\fn vfs_dir_list_t* dev_append_elem
 *			(inode_t inode, char* name, vfs_dir_list_t* lst)
 *	\brief Helper function to build a directory field list.
 *	\param inode Inode to add in the list
 * 	\param name Entry name in the directory
 * 	\param lst Current list.
 *	\return A pointer to the first element of the list.
 */
vfs_dir_list_t* dev_append_elem (inode_t inode, char* name, vfs_dir_list_t* lst) {
    vfs_dir_list_t* res = malloc(sizeof(vfs_dir_list_t));
    res->name = malloc(strlen(name)+1);
    strcpy(res->name, name);
    res->inode = inode;
    res->next = lst;
    return res;
}


/**	\fn vfs_dir_list_t* dev_lsdir (inode_t from)
 *	\brief List the content of a directory.
 *	\param from The explored inode.
 *	\return On success: a pointer to the first element of the list.
 *			On fail: a NULL pointer, with errno set to the appropriate value.
 */
vfs_dir_list_t* dev_lsdir (inode_t from) {
    if (from.st.st_ino == DEV_ROOT) {
        vfs_dir_list_t* res = NULL;

        inode_t r;
        r.st.st_ino = DEV_FB;
        r.st.st_mode = S_IFREG | S_IRWXU | S_IRWXO | S_IRWXG;
        r.st.st_size = kernel_framebuffer->bufferSize;
        r.sb = from.sb;
        r.op = &dev_inode_operations;
        res = dev_append_elem(r, "fb", res);

        r.st.st_size = 0;
        r.st.st_ino = DEV_NULL;
        r.st.st_mode = S_IFCHR | S_IRWXU | S_IRWXO | S_IRWXG;
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

		r.st.st_mode = S_IFDIR;
        r.st.st_ino = DEV_ROOT;
        res = dev_append_elem(r, ".", res);

		r.st.st_mode = S_IFDIR;
        r.st.st_ino = DEV_ROOT;
        res = dev_append_elem(r, "..", res);
        return res;
    } else {
		kernel_printf("ls on a dev file.\n");
	}
	errno = ENOENT;
    return NULL;
}

/**	\fn int dev_fread (inode_t from, char* buf, int size, int pos)
 *	\brief Read the inode from offset 'position', maximum size fixed, into 'buf'.
 *	\param from The read inode.
 *	\param buf Destination buffer.
 *	\param size Buffer maximum size.
 *	\param pos File read offset.
 *	\return On success: the number of bytes read.
 *			On fail: -1, with errno set to the appropriate value.
 */
int dev_fread (inode_t from, char* buf, int size, int pos) {
    (void)pos;
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
		case DEV_FB:
			if (size+pos > kernel_framebuffer->bufferSize) {
				size = kernel_framebuffer->bufferSize - pos;
			}
			if (size > 0 && fb_has_window(get_current_process_id())) {
				memcpy(buf, kernel_framebuffer->bufferPtr+fb_offset(get_current_process_id())+pos, size);
				return size;
			}
			return 0;
    }
	errno = EINVAL;
    return -1;
}

/**	\fn int dev_fwrite (inode_t from, char* buf, int size, int pos)
 *	\brief Write the inode from offset 'position', maximum size fixed, from 'buf'.
 *	\param from The written inode.
 *	\param buf Source buffer.
 *	\param size Buffer maximum size.
 *	\param pos File read offset.
 *	\return On success: the number of bytes written.
 *			On fail: -1, with errno set to the appropriate value.
 */
int dev_fwrite(inode_t from, char* buf, int size, int pos) {
    (void)pos;
    switch (from.st.st_ino) {
        case DEV_SERIAL:
            for (int i=0;i<size;i++) {
                serial_putc(buf[i]);
            }
            return size;
		case DEV_FB:
			if (size+pos > kernel_framebuffer->bufferSize) {
				size = kernel_framebuffer->bufferSize - pos;
			}
			int pid = get_current_process_id();
			if (size > 0 && fb_has_window(pid)) {
				if ((!fb_is_buffered(pid)) && fb_has_focus(pid)){
					memcpy(kernel_framebuffer->bufferPtr+pos, buf, size);
				}
				memcpy(kernel_framebuffer->bufferPtr+fb_offset(get_current_process_id())+pos, buf, size);
				return size;
			} else {
				return 0;
			}
        default:
			errno = EINVAL;
            return -1;
    }
}
