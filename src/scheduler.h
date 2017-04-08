#include "process.h"
#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * Schedule the active processes
 */

#define MAX_PROCESSES 10

void setup_scheduler();
int get_next_process();
int kill_process(int const process_id);
int create_process();

int get_number_active_processes();

process* get_process_list();
int* get_active_processes();


#endif //SCHEDULER_H
