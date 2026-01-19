#include "multitasking/scheduler.h"
#include "kernel/print.h"
#include "kernel/panic.h"
#include "mm/kheap.h"
#include "utils.h"

/* define in process */
static process_t * idle_process;
static process_t * processes_list;
static process_t * current_process;

/* Note: a system design is that current process can never be NULL
   it may always have the idle process */

void scheduler_init() {
    void idle_process_main();

    idle_process = process_create(PROCESS_IDLE, idle_process_main, 0x1000); /* idle thread stack doesn't need to be very long */
    current_process = idle_process;
    processes_list = NULL;
}

void scheduler_add_processes_to_list(process_t * process) {
    if (processes_list == NULL) {
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
    process_t *current = processes_list;

    while (current) {
        if (current->status == PROCESS_ZOMBIE && current != current_process) {
            process_t *to_delete = current;

            /* unlink */
            if (prev) {
                prev->next = current->next;
            } else {
                /* removing head */
                processes_list = current->next;
            }

            current = current->next;

            /* free zombie */
            kfree(to_delete);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

process_t * scheduler_get_next_process() {
    process_t *p = current_process->next;

    if (!p)
        p = processes_list;

    while (p) {
        if (p->status == PROCESS_READY)
            return p;
        p = p->next;
    }

    return idle_process;
}

void scheduler_schedule() {
    /* first of all remove all zombie process
       so next process wouldn't be a zombie status kind */
    scheduler_remove_zombie_processes();

    /* Fix: There is a need to check what will happen if current process = next process */
    process_t * next_process = scheduler_get_next_process();
    process_t * current_process_copy = current_process;

    current_process->status = PROCESS_READY;
    next_process->status = PROCESS_RUNNING;

    current_process = next_process;
    
    scheduler_context_switch_asm(&current_process_copy->esp, next_process->esp);
}

void scheduler_thread_exit() {
    process_t * next_process = scheduler_get_next_process();
    process_t * current_process_copy = current_process;

    if (current_process->type == PROCESS_IDLE) PANIC("The idle thread can't exit!!!");
    current_process->status = PROCESS_ZOMBIE;
    next_process->status = PROCESS_RUNNING;

    current_process = next_process;

    scheduler_context_switch_asm(NULL, current_process->esp);
}