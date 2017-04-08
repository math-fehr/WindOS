#include "process.h"
#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * Schedule the active processes
 */

#define MAX_PROCESSES 10

static process process_list[MAX_PROCESSES];
static int active_processes[MAX_PROCESSES];
static int free_processes[MAX_PROCESSES];

static int current_process;
static int number_active_processes;
static int number_free_processes;

void setup_scheduler();
int get_next_process();
int kill_process(int const process_id);
int create_process();
void test_scheduler();

#endif //SCHEDULER_H
