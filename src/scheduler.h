#include "process.h"
#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * Scheduler of the processes
 */

/**
 * Maximum number of processes that can be handled
 */
#define MAX_PROCESSES 42

/**
 * Setup the scheduler
 */
void setup_scheduler();

/**
 * Gets the ID of the next process to analyse
 * Returns -1 if there is no active process
 */
int get_next_process();

/**
 * Kill a process with its id
 * Return 0 is the attempt is successfull
 * Return -1 if the process does not exist
 */
int kill_process(int const process_id);

/**
 * Create a process
 * The function return -1 if there is no free_process available
 */
int create_process();

/**
 * Get number of active processes
 */
int get_number_active_processes();

/**
 * Get the list of processes
 */
process* get_process_list();

/**
 * Get the number of active processes
 */
int* get_active_processes();


#endif //SCHEDULER_H
