#ifndef PROCESS_H
#define PROCESS_H

/**
 * Represent an active process
 */

/**
 * status of a process
 */
typedef enum {
    status_active,
    status_free,
    status_zombie
} status_process;

/**
 * An process (running or not)
 */
typedef struct {
    status_process status;
    int dummy; //Temp value for debug
} process;

#endif //PROCESS_H
