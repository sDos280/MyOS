#include "multitasking/scheduler.h"
#include "kernel/print.h"
#include "kernel/panic.h"
#include "mm/kheap.h"
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

static void scheduler_remove_zombie_processes() {
    process_t *prev = NULL;
    process_t *p = processes_list;

    while (p) {
        if (p->status == PROCESS_ZOMBIE && p != current_process) {
            process_t *to_delete = p;

            /* unlink */
            if (prev) {
                prev->next = p->next;
            } else {
                /* removing head */
                processes_list = p->next;
            }

            p = p->next;

            /* free zombie */
            kfree(to_delete);
        } else if (p->status == PROCESS_ZOMBIE && p == current_process) {
            PANIC("The idle thread can't exit!!!");
        } else {
            prev = p;
            p = p->next;
        }
    }
}

process_t * scheduler_get_next_process() {
    if (current_process == NULL) return NULL;

    scheduler_remove_zombie_processes();

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

void scheduler_thread_exit() {
    process_t * next_process = scheduler_get_next_process();
    process_t * current_process_copy = current_process;

    if (next_process == NULL) PANIC("The idle thread can't exit!!!");
    current_process->status = PROCESS_ZOMBIE;
    next_process->status = PROCESS_RUNNING;

    current_process = next_process;

    scheduler_start_thread_asm(current_process_copy->esp);
}