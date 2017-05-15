#include "procfs.h"


static inode_operations_t proc_inode_operations = {
  .read_dir = proc_lsdir,
  .read = proc_fread,
};

/**	\fn superblock_t* proc_initialize(int id)
 *	\brief Initialize a virtual process directory.
 *	\param id Virtual FS identifier.
 */
superblock_t* proc_initialize(int id) {
	superblock_t* res = malloc(sizeof(superblock_t));
    res->id = id;
    res->root.st.st_ino    = 2;
    res->root.st.st_size   = 1024;
    res->root.st.st_mode   = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXG;
    res->root.st.st_dev    = id;
	res->root.sb 		   = res;
    res->root.op           = &proc_inode_operations;
    return res;
}

/**	\fn vfs_dir_list_t* proc_lsdir(inode_t from)
 *	\brief List the process directory.
 */
vfs_dir_list_t* proc_lsdir(inode_t from) {
    if (from.st.st_ino == 2) {
		vfs_dir_list_t* res = NULL;
        inode_t r;
		r.st.st_ino = 2;
        r.st.st_mode = S_IFREG | S_IRUSR | S_IROTH | S_IRGRP;
        r.st.st_size = 69;
        r.sb = from.sb;
        r.op = &proc_inode_operations;


		for (int i=0;i<MAX_PROCESSES;i++) {
			process** list = get_process_list();
			process* p = list[i];
			if (p != NULL) {
		        r.st.st_ino = p->asid + 3;
				char buf[10];
				sprintf(buf, "%d", p->asid);
		        res = dev_append_elem(r,buf,res);
			}
		}

		r.st.st_mode = S_IFDIR;
        r.st.st_ino = 2;
        res = dev_append_elem(r, ".", res);

		r.st.st_mode = S_IFDIR;
        r.st.st_ino = 2;
        res = dev_append_elem(r, "..", res);
		return res;
	} else {
		errno = ENOENT;
		return NULL;
	}
}

/**	\fn int proc_fread(inode_t from, char* buf, int size, int pos)
 *	\brief Read process data.
 *	\param from Inode representing a process.
 *	\param buf Destination buffer.
 *	\param size Size buffer.
 *	\param pos Offset.
 */
int proc_fread(inode_t from, char* buf, int size, int pos) {
    if (from.st.st_ino == 2) {
		errno = EISDIR;
		return -1;
	} else {
		int pid = from.st.st_ino - 3;
		if (pid < 0 || pid >= MAX_PROCESSES) {
			errno = ENOENT;
			return -1;
		} else {
			process** list = get_process_list();
			process* p = list[pid];
			if (p != NULL) {
				char data_buffer[1024];
				data_buffer[0] = 0;
				char str_state[2];
				str_state[1] = 0;
				switch (p->status) {
					case status_active:
						str_state[0] = 'R';
						break;
					case status_blocked_svc:
					case status_wait:
						str_state[0] = 'S';
						break;
					case status_zombie:
						str_state[0] = 'Z';
						break;
				}
				int n = sprintf(data_buffer, "Name: % -32s\nState:  %s\nPID: % 4d\nPPID: % 3d\n",
							p->name,
							str_state,
							p->asid,
							p->parent_id);
				if (pos < n) {
					strncpy(buf, data_buffer+pos, size);
					return min(n-pos,size);
				} else {
					return 0;
				}
			} else {
				errno = ENOENT;
				return -1;
			}
		}
	}
}
