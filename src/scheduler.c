/** \file scheduler.c
 *	\brief Scheduling features.
 */

#include "scheduler.h"
#include "serial.h"
#include "debug.h"
#include "string.h"

/** \var process* process_list[MAX_PROCESSES]
 *	\brief List of possible processes (not all are active)
 */
static process* process_list[MAX_PROCESSES];

/** \var int active_processes[MAX_PROCESSES]
 *	\brief List of active processes (only the first number_active_processes)
 */
static int active_processes[MAX_PROCESSES];

/** \var int free_processes[MAX_PROCESSES];
 * 	\brief List of free processes (only the first number_free_processes)
 */
static int free_processes[MAX_PROCESSES];

/** \var int zombie_processes[MAX_PROCESSES];
 * 	\brief List of zombie processes (only the first number_zombie_processes)
 */
static int zombie_processes[MAX_PROCESSES];

/** \var int current_process_id
 * 	\brief Index of current process in active_processes.
 */
static int current_process_id;

/** \var int number_active_processes
 * 	\brief Active processes count.
 */
static int number_active_processes;

/** \var int number_free_processes
 * 	\brief Free processes count.
 */
static int number_free_processes;

/** \var int number_zombie_processes
 * 	\brief Zombie processes count.
 */
static int number_zombie_processes;


/** \fn void setup_scheduler()
 *	\brief Initialize scheduler global variables.
 */
void setup_scheduler() {
    kernel_printf("[SHED] Scheduler set up!\n");
    current_process_id = -1;
    number_active_processes = 0;
	number_zombie_processes = 0;
    number_free_processes = MAX_PROCESSES;
    for(int i = 0; i<MAX_PROCESSES; i++) {
        process_list[i] = NULL;
        free_processes[i] = MAX_PROCESSES-i-1;
    }
}

/** \fn process* get_next_process()
 * 	\brief Find the next process in the execution list.
 *	\return A pointer to the next process on succes. NULL pointer on fail.
 */
process* get_next_process() {
    if(number_active_processes == 0) {
        return NULL;
    }

    if(current_process_id < 0) {
        current_process_id = 0;
    }
    else {
        current_process_id++;
    }
    if(current_process_id >= number_active_processes) {
        current_process_id = 0;
    }
	process_list[active_processes[current_process_id]]->dummy++;

//kernel_printf("\033[s\033[%d;%dH%d\033[u", 1, 1, active_processes[current_process_id]);
    return process_list[active_processes[current_process_id]];
}

extern uint32_t __ram_size;

/**	\fn void free_process_data (process* p)
 *	\param p The process data to free.
 *	\brief Free all the allocated memory of a process.
 */
void free_process_data(process* p) {
	// Free program break.
	int n_allocated_pages = p->brk_page;
	for (int i=n_allocated_pages;i>0;i--) {
		int phy_page = mmu_vir2phy_ttb(i*PAGE_SECTION, p->ttb_address) / PAGE_SECTION; // let's hope GCC optimizes this
		paging_free(1,phy_page);
	}

	// free stack and program code
	paging_free(1,mmu_vir2phy_ttb(0, p->ttb_address)/PAGE_SECTION);
	paging_free(1,mmu_vir2phy_ttb(__ram_size-PAGE_SECTION, p->ttb_address)/PAGE_SECTION);

	for (int i=0;i<64;i++) {
		if (p->fd[i].position >= 0) {
			free_vfs_dir_list(p->fd[i].dir_entry);
			free(p->fd[i].inode);
		}
	}

	free((void*)p->ttb_address);
	free(p);
}

/** \fn int kill_process(int const process_id, int wstatus)
 *	\brief Kill a process.
 *	\param process_id PID to kill.
 * 	\param wstatus Kill status.
 * 	\return 0 on success. -1 on failure.
 *
 *	When killing a process, two things can happen:
 * 	- if the parent was waiting for his death, the process is immediately freed,
 *	and the parent is notified.
 *	- if the parent isn't, the process is put in zombie mode.
 */
int kill_process(int const process_id, int wstatus) {
	if (process_id >= MAX_PROCESSES || process_id < 0 || process_list[process_id] == NULL) {
		return -1;
	}

	process* child  = process_list[process_id];
	process* parent = process_list[child->parent_id];
	for (int i=0;i<MAX_PROCESSES;i++) {
		if ((process_list[i] != NULL) && (process_list[i]->parent_id == process_id)) {
			process_list[i]->parent_id = 0;

/*			if (process_list[i]->status == status_zombie &&  (process_list[0]->wait.pid == -1 || process_list[0]->wait.pid == process_list[i]->asid)) {
				kernel_printf("Reaped by init.\n");
				process_list[0]->status 		= status_active;
				process_list[0]->ctx.r[0] 		= process_list[i]->asid;
				if (process_list[0]->wait.wstatus != NULL) {
					int* phy_addr = (int*)(0x80000000+mmu_vir2phy_ttb((intptr_t)process_list[0]->wait.wstatus, process_list[0]->ttb_address));
					*phy_addr = wstatus;
				}

				int j=0;
				for(;zombie_processes[j] != i;j++) {}

				zombie_processes[j] = zombie_processes[number_zombie_processes-1];
				number_zombie_processes--;

				active_processes[number_active_processes] = 0;
				number_active_processes++;

				free_processes[number_free_processes] = process_list[i]->asid; // add it into the free list
				number_free_processes++;

				free_process_data(process_list[i]);
				process_list[process_list[i]->asid] = NULL;
			}*/
		}
	}

	if (parent->status == status_wait && (parent->wait.pid == -1 || parent->wait.pid == process_id)) {
		// parent was waiting for his death, free the process and notify parent.
		parent->status 		= status_active;
		parent->ctx.r[0] 	= process_id;
		if (parent->wait.wstatus != NULL) {
			int* phy_addr = (int*)(0x80000000+mmu_vir2phy_ttb((intptr_t)parent->wait.wstatus, parent->ttb_address));
			*phy_addr = wstatus;
		}

		active_processes[number_active_processes] = child->parent_id;
		number_active_processes++;

		free_processes[number_free_processes] = process_id; // add it into the free list
		number_free_processes++;

		free_process_data(child);
		process_list[process_id] = NULL;
	} else {
		// parent doesn't care, so let's put the child into zombie mode.
		child->status = status_zombie;
		child->wait.wstatus = (int*)wstatus;

		zombie_processes[number_zombie_processes] = process_id;
		number_zombie_processes++;
	}
	int i=0;
	for (;active_processes[i] != process_id;i++) {} // Danger
	active_processes[i] = active_processes[number_active_processes-1];
	number_active_processes--;
    return 0;
}

/** \fn int wait_process(int const process_id, int target_pid, int* wstatus)
 *	\brief Wait for a process.
 *	\param process_id The waiting process.
 *	\param target_pid The waited process.
 *	\param wstatus Target pointer to return exit status.
 *
 *	When waiting for a process, two things can happen:
 *	- if a zombie child is found, free him and return immediately to parent.
 *	- if not, put the parent in wait status.
 */
int wait_process(int const process_id, int target_pid, int* wstatus) {
	if (process_id >= MAX_PROCESSES || process_id < 0 || process_list[process_id] == NULL) {
		return -1;
	}

	process* parent = process_list[process_id];
	int* phy_addr = (int*)(0x80000000+mmu_vir2phy_ttb((intptr_t)wstatus, parent->ttb_address));

	if (target_pid == -1) { // Reap a zombie children.
		for (int i=0;i<number_zombie_processes;i++) {
			process* child = process_list[zombie_processes[i]];
			if (child->parent_id == process_id) {
				// we found one.
				int child_pid = child->asid;
				zombie_processes[i] = zombie_processes[number_zombie_processes-1];
				number_zombie_processes--;
				free_processes[number_free_processes] = child_pid;
				number_free_processes++;
				process_list[child_pid] = 0;
				if (wstatus != NULL)
					*phy_addr = (int)child->wait.wstatus;

				free_process_data(child);
				return child_pid;
			}
		}
	} else { // TODO: more control.
		process* child  = process_list[target_pid];
		if (child->status == status_zombie && child->parent_id == process_id) {
			int i=0;
			for (;zombie_processes[i] != target_pid;i++) {} // Danger

			zombie_processes[i] = zombie_processes[number_zombie_processes-1];
			number_zombie_processes--;
			free_processes[number_free_processes] = target_pid;
			number_free_processes++;
			process_list[target_pid] = 0;

			if (wstatus != NULL)
				*phy_addr = (int)child->wait.wstatus;
			free_process_data(child);
			return target_pid;
		}
	}

	// No zombie found: parent goes in wait status.

	int i=0;
	for (;active_processes[i] != process_id;i++) {} // Danger
	active_processes[i] = active_processes[number_active_processes-1];
	number_active_processes--;
	parent->status 		 = status_wait;
	parent->wait.pid 	 = target_pid;
	parent->wait.wstatus = wstatus;

	return -1;
}

/** \fn int sheduler_add_process(process* p)
 *  \brief Put a process in the scheduling structure.
 *	\param p The process to add.
 *	\return The id given to this process.
 */
int sheduler_add_process(process* p) {
    if (number_free_processes == 0) {
        return -1;
    }
    int new_process_id = free_processes[number_free_processes-1];
    number_free_processes--;
    active_processes[number_active_processes] = new_process_id;
    number_active_processes++;
    process_list[new_process_id] = p;
    p->asid = new_process_id;

    return new_process_id;
}

int get_number_active_processes() {
    return number_active_processes;
}

process** get_process_list() {
    return process_list;
}


int* get_active_processes() {
    return active_processes;
}

process* get_current_process() {
    if(current_process_id< 0) {
        return NULL;
    }
    return process_list[active_processes[current_process_id]];
}

int get_current_process_id() {
    if(current_process_id < 0) {
        return -1;
    }
    return active_processes[current_process_id];
}
