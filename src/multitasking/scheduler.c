#include "multitasking/scheduler.h"
#include "kernel/print.h"
#include "kernel/panic.h"
#include "mm/kheap.h"
#include "utils.h"

/* define in process */
static process_t * idle_process;
static process_t * processes_ready_queue;
static process_t * processes_zombie_queue;
static process_t * current_process;

static void add_to_process_queue(process_t ** pqueue, process_t * element) {
    if (pqueue == NULL) return;

    if (element->next != NULL) PANIC("Added elements to the queues should not be entangled");

    /* the queue is empty */
    if (*pqueue == NULL) {
        *pqueue = element;
        return;
    }

    /* get the last element in the queue */
    process_t * last = *pqueue;

    while (last->next != NULL)
        last = last->next;

    last->next = element;
}

static process_t * remove_to_process_queue(process_t ** pqueue) {
    if (pqueue == NULL) return NULL;

    /* the queue is empty */
    if (*pqueue == NULL) return NULL;

    process_t * p = *pqueue;
    *pqueue = p->next; /* note, this may set the queue as empty <=> p.next = NULL */

    p->next = NULL;

    return p;
}

/* Note: a system design is that current process can never be NULL
   it may always have the idle process */
void scheduler_init() {
    void idle_process_main();

    idle_process = process_create(PROCESS_IDLE, idle_process_main, 0x1000); /* idle thread stack doesn't need to be very long */
    current_process = idle_process;
    processes_ready_queue = NULL;
    processes_zombie_queue = NULL;
}

void scheduler_add_process_to_ready_queue(process_t * process) {
    process->status = PROCESS_READY;

    add_to_process_queue(&processes_ready_queue, process);
}

static void scheduler_remove_zombie_processes() {
    process_t * p = remove_to_process_queue(&processes_zombie_queue);

    while (p != NULL) {
        kfree(p);
        p = remove_to_process_queue(&processes_zombie_queue);
    }
}

process_t * scheduler_get_next_process() {
    /* Important: may return the same process */
    process_t * p = remove_to_process_queue(&processes_ready_queue);

    /* If there is no running process and the running process isn't the idle one, we would want to continue on this 
       process */
    if (p == NULL && current_process->type != PROCESS_IDLE) {
        return current_process;
    } else if (p == NULL) {
        return idle_process;
    }

    return p;
}

void scheduler_schedule() {
    /* first of all remove all zombie process
       so next process wouldn't be a zombie status kind */
    scheduler_remove_zombie_processes();

    /* Fix: There is a need to check what will happen if current process = next process */
    process_t * next_process = scheduler_get_next_process();

    /* there is no need to shedule if the next process is the same */
    if (current_process == next_process) return;
    
    process_t * current_process_copy = current_process;

    current_process->status = PROCESS_READY;

    if (current_process->type != PROCESS_IDLE)
        add_to_process_queue(&processes_ready_queue, current_process); /* add the process to the end of the queue */
    
    next_process->status = PROCESS_RUNNING;
    current_process = next_process;
    
    scheduler_context_switch_asm(&current_process_copy->esp, next_process->esp);
}

void scheduler_thread_exit() {
    if (current_process->type == PROCESS_IDLE) PANIC("The idle thread can't exit!!!");

    process_t * next_process = scheduler_get_next_process();

    /* if the next process equals to the current process then we need to set the current process to idle */
    if (next_process == current_process) {
        current_process = idle_process;
        current_process->status = PROCESS_RUNNING;
        goto thread_exit_switch;
    }

    process_t * current_process_copy = current_process;
    
    current_process->status = PROCESS_ZOMBIE;
    /* Note: the current running process, shouldn't be linked in any of the queues */
    add_to_process_queue(&processes_zombie_queue, current_process);
    next_process->status = PROCESS_RUNNING;

    current_process = next_process;

thread_exit_switch:
    scheduler_context_switch_asm(NULL, current_process->esp);
}