#include "scheduler.h"

#include "serial.h"
#include "debug.h"
#include "string.h"

void test_scheduler() {
    char command[256];
    int n = 5;
    setup_scheduler();
    for(int i = 0; i<n; i++) {
        create_process();
    }
    while(1) {
        serial_readline(command,256);
        char* token = strtok(command," ");
        if(!strcmp("display",command)) {
            for(int i = 0; i<number_active_processes; i++) {
                kernel_printf("pid: %d    dummy: %d",active_processes[i],process_list[active_processes[i]].dummy);
                if(process_list[active_processes[i]].status == status_zombie) {
                    kernel_printf(" zombie");
                }
                serial_newline();
            }
        }
        else if(!strcmp("create",token)) {
            int number = atoi(strtok(NULL," "));
            int i = 0;
            bool error = false;
            while(i < number && !error) {
                int temp = create_process();
                if(temp == -1) {
                    error = true;
                }
                i++;
            }
            if(error) {
                serial_write("Limit of processes reached");
                serial_newline();
            }
        }
        else if(!strcmp("kill",token)) {
            int number = atoi(strtok(NULL," "));
            serial_newline();
            int temp= kill_process(number);
            if(temp == -1) {
                serial_write("Can't kill that process");
                serial_newline();
            }
        }
        else if(!strcmp("step",token)) {
            int number = atoi(strtok(NULL," "));
            bool error = false;
            for(int i = 0; i<number && !error; i++) {
                int pid = get_next_process();
                if(pid == -1) {
                    error = true;
                }
                else {
                    process_list[pid].dummy++;
                }
            }
        }
        else {
            serial_write("Unrecognized command");
            serial_newline();
        }
        serial_newline();
    }
}

void setup_scheduler() {
    current_process = 0;
    number_active_processes = 0;
    number_free_processes = MAX_PROCESSES;
    for(int i = 0; i<MAX_PROCESSES; i++) {
        process_list[i].status = status_free;
        process_list[i].dummy = 0;
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
    while(process_list[active_processes[current_process]].status == status_zombie) {
        process_list[active_processes[current_process]].status = status_free;
        process_list[active_processes[current_process]].dummy = 0;
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
    if(process_id < MAX_PROCESSES && process_list[process_id].status == status_active) {
        process_list[process_id].status = status_zombie;
        return 0;
    }
    return -1;
}


int create_process() {
    if(number_free_processes == 0) {
        return -1;
    }
    int new_process_id = free_processes[number_free_processes-1];
    number_free_processes--;
    active_processes[number_active_processes] = new_process_id;
    number_active_processes++;
    process_list[new_process_id].dummy = 0;
    process_list[new_process_id].status = status_active;
    return new_process_id;
}
