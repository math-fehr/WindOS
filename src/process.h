#ifndef PROCESS_H
#define PROCESS_H

/**
 * Represent an active process
 */

typedef enum {
    status_active,
    status_free,
    status_zombie
} status_process;


typedef struct {
    status_process status;
    int dummy; //Temp value
} process;

#endif //PROCESS_H
