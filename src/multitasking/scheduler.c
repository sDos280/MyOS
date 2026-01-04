#include "multitasking/scheduler.h"
#include "kernel/print.h"
#include "kernel/panic.h"
#include "utils.h"

#define SHEDULER_ON  1
#define SHEDULER_OFF 0

/* define in process */
extern process_t * processes_list;
extern process_t * current_process;
static uint8_t state = SHEDULER_OFF;

void sheduler_run() {
    state = SHEDULER_ON;
}

cpu_status_t scheduler_schedule(cpu_status_t * context) {
    static size_t sheduled_count = 0; /* the number of time we have shedule a process */

    if (state == SHEDULER_ON) {
        /* Note: current_process must be set, and processes_list not empty */
        /* Fix: the first time the sheduler move from kernel execution to process execution
                the kernel stack is currpted (since there is no pace to save the current context) */

        if ((sheduled_count++) != 0) {
            /* this is not the first process we will shedule */
            // current_process->context = *context;  // save the current process this create problems
            memcpy(&current_process->context, context, sizeof(cpu_status_t)); /* save the current process context */
            
            /* move to the next provess */
            if (current_process->next != NULL) 
                current_process = current_process->next;
            else 
                current_process = processes_list;
        }

        return current_process->context;
    } else {
        return *context;
    }
}