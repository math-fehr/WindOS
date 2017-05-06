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
process** get_process_list();

/**
 * Get the number of active processes
 */
int* get_active_processes();


#endif //SCHEDULER_H
