#include "scheduler.h"

#include "serial.h"
#include "debug.h"
#include "string.h"

//The list of possible processes (not all are active)
static process* process_list[MAX_PROCESSES];
//The list of active processes (only the first number_active_processes are active)
static int active_processes[MAX_PROCESSES];
//The list of free processes (only the first number_free_processes are active)
static int free_processes[MAX_PROCESSES];
//Zombie processes
static int zombie_processes[MAX_PROCESSES];

//The current process running in active_processes
static int current_process_id;

//The number of active processes
static int number_active_processes;
//The number of free processes
static int number_free_processes;
// Zombie count
static int number_zombie_processes;


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

void free_process_data(process* p) {
	// Free program break.
	int n_allocated_pages = p->brk_page;
	for (int i=n_allocated_pages;i>0;i--) {
		int phy_page = mmu_vir2phy(i*PAGE_SECTION) / PAGE_SECTION; // let's hope GCC optimizes this
		paging_free(1,phy_page);
	}

	// free stack and program code
	paging_free(1,mmu_vir2phy(0)/PAGE_SECTION);
	paging_free(1,mmu_vir2phy(__ram_size-PAGE_SECTION)/PAGE_SECTION);

	free((void*)p->ttb_address);
	free(p);
}

int kill_process(int const process_id, int wstatus) {
	if (process_id >= MAX_PROCESSES || process_id < 0 || process_list[process_id] == NULL) {
		return -1;
	}

	process* child  = process_list[process_id];
	process* parent = process_list[child->parent_id];

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
				zombie_processes[i] = zombie_processes[number_zombie_processes];
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

			zombie_processes[i] = zombie_processes[number_zombie_processes];
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
