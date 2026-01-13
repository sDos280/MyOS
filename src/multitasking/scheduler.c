#include "multitasking/scheduler.h"
#include "kernel/print.h"
#include "kernel/panic.h"
#include "utils.h"

#define SHEDULER_ON  1
#define SHEDULER_OFF 0

/* define in process */
static process_t * processes_list;
static process_t * current_process;

void scheduler_add_processes_to_list(process_t * process) {
    if (current_process == NULL || processes_list == NULL) {
        current_process = process;
        processes_list = process;
        return;
    }

    process_t * last = processes_list;

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = process;
}

process_t * scheduler_get_next_process() {
    if (current_process == NULL) return NULL;

    if (current_process->next == NULL) return processes_list;

    return current_process->next;
}

void schduler_schedule() {
    /* Fix: There is a need to check what will happen if current process = next process */
    process_t * next_process = scheduler_get_next_process();
    process_t * current_process_copy = current_process;

    if (next_process == NULL) return;
    current_process->status = PROCESS_READY;
    next_process->status = PROCESS_RUNNING;

    current_process = next_process;
    
    scheduler_context_switch_asm(&current_process_copy->esp, next_process->esp);
}