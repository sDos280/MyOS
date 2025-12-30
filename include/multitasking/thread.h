#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include "tty.h"
#include "paging.h"

#define THREAD_MAX_THREADS 50

typedef enum {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_ZOMBIE
} thread_state_e;

typedef uint32_t tid_t;

typedef struct thread_control_struct {
    void * cr3;       /* the physical address of the thread page table */
    void * eip;       /* the next instruction in vm to execute in the thead */
    // uint32_t eflags;  /* cpu state */
    uint32_t eax, ecx, edx, ebx, 
             esp, ebp, esi, edi;  /* general purpose registors */
    /* Note: no need to save segmentation registors, 
             since this os doesn't use segmentation mechanism */
    tty_t * tty;  /* the terminal handler */
} thread_control_t;

typedef struct thread_struct {
    tid_t tid;
    thread_state_e state;  /* the state of the thread */

    thread_control_t context;

    uint32_t time_slice;
    // uint32_t wakeup_tick;
} thread_t;

void thread_init();
thread_t * thread_create(void (*entry)(void *), size_t stack_size, tty_t * tty);
uint8_t thread_announce(thread_t * thread); /* 1 on error, else 0 */
thread_t * thread_cycle_to_next_ready_thread(); /* return the next ready thread, return NULL if now found */

#endif // THREAD_H