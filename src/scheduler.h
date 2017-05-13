#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
/**
 * Scheduler of the processes
 */


/**
 * Setup the scheduler
 */
void setup_scheduler();


/*
 * Add a process in the table
 * Returns the id of the process on sucess, -1 on error
 */
int sheduler_add_process(process* p);

/**
 * Gets the next process to analyse
 * Returns NULL if there is no active process
 */
process* get_next_process();

/**
 * Kill a process with its id
 * Return 0 is the attempt is successfull
 * Return -1 if the process does not exist
 */
int kill_process(int const process_id, int wstatus);

/**
 * process_id waits for target_pid to go into zombie state
 */
int wait_process(int const process_id, int target_pid, int* wstatus);

/**
 * Get number of active processes
 */
int get_number_active_processes();

/**
 * Get the list of processes
 */
process** get_process_list();

/**
 * Get the number of active processes
 */
int* get_active_processes();

/**
 * Return a pointer to the current process
 * Return NULL if no process are running
 */
process* get_current_process();

/**
 * Return the id of the current process
 * Return -1 if no process are running
 */
int get_current_process_id();

#endif //SCHEDULER_H
