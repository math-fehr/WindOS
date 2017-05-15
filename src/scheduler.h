#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"



void setup_scheduler();
int sheduler_add_process(process* p);
process* get_next_process();
int kill_process(int const process_id, int wstatus);
int wait_process(int const process_id, int target_pid, int* wstatus);
int get_number_active_processes();
process** get_process_list();
int* get_active_processes();
process* get_current_process();
int get_current_process_id();

#endif //SCHEDULER_H
