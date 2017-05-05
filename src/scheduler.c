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

//The current process running
static int current_process;
//The number of active processes
static int number_active_processes;
//The number of free processes
static int number_free_processes;


void setup_scheduler() {
    kernel_printf("[SHED] Scheduler set up!\n");
    current_process = 0;
    number_active_processes = 0;
    number_free_processes = MAX_PROCESSES;
    for(int i = 0; i<MAX_PROCESSES; i++) {
        process_list[i] = 0;
        free_processes[i] = MAX_PROCESSES-i-1;
    }
}


int get_next_process() {
    current_process++;
    if(current_process == number_active_processes) {
        current_process = 0;
    }
    if(number_active_processes == 0) {
        return -1;
    }
    while(process_list[active_processes[current_process]]->status == status_zombie) {
        process_list[active_processes[current_process]]->status = status_free;
        process_list[active_processes[current_process]]->dummy = 0;
        free_processes[number_free_processes] = active_processes[current_process];
        active_processes[current_process] = active_processes[number_active_processes-1];
        number_active_processes--;
        number_free_processes++;
        if(current_process == number_active_processes) {
            current_process = 0;
        }
        if(number_active_processes == 0) {
            return -1;
        }
    }
    return active_processes[current_process];
}


int kill_process(int const process_id) {
    if(process_id < MAX_PROCESSES && process_list[process_id]->status == status_active) {
        process_list[process_id]->status = status_zombie;
        return 0;
    }
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

int create_process() {
    if(number_free_processes == 0) {
        return -1;
    }
    int new_process_id = free_processes[number_free_processes-1];
    number_free_processes--;
    active_processes[number_active_processes] = new_process_id;
    number_active_processes++;
    process_list[new_process_id] = malloc(sizeof(process));
    process_list[new_process_id]->dummy = 0;
    process_list[new_process_id]->status = status_active;
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
