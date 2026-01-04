#ifndef THREAD_H
#define THREAD_H

#include "kernel/tty.h"
#include "kernel/description_tables.h"
#include "types.h"

typedef enum process_state_enum {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_SLEEPING,
    PROCESS_ZOMBIE
} process_state_e;

typedef size_t pid_t;

typedef struct process_sturct {
    pid_t pid;
    process_state_e status;
    cpu_status_t context;
    struct process_sturct * next;
} process_t;

void process_init();
process_t * process_create(void (*entry)(void *), size_t stack_size);
uint8_t process_announce(process_t * process);
uint8_t process_set_current(process_t * process); /* the process must be annonced */

#endif // THREAD_H