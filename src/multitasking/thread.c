#include "multitasking/thread.h"
#include "mm/kheap.h"
#include "mm/paging.h"
#include "kernel/print.h"
#include "utils.h"

static tid_t next_tid = 1;

static thread_t * threads[THREAD_MAX_THREADS];
static size_t current_thread_index;

void thread_init() {
    memset(threads, 0, sizeof(thread_t *) * THREAD_MAX_THREADS);
    current_thread_index = 0;
}

thread_t * thread_create(void (*entry)(void *), size_t stack_size, tty_t * tty) {
    thread_t * out = kalloc(sizeof(thread_t));

    if (out == NULL) return NULL; /* no more memory or error */
    
    memset(out, 0, sizeof(thread_t));

    page_directory_t * current_dir = paging_get_current_directory();

    out->tid = next_tid;
    next_tid++;
    out->state = THREAD_NEW;
    out->context.cr3 = current_dir->physical_addr;
    out->context.eip = entry;
    out->context.eax = 0; out->context.ecx = 0; out->context.edx = 0; out->context.ebx = 0; 
    out->context.esp = 0; out->context.ebp = 0; out->context.esi = 0; out->context.edi = 0;

    tty_t * ctty = print_get_tty();

    out->context.tty;

    out->time_slice = 0;  // Fix: should update this, should improve the timer module
}

uint8_t thread_announce(thread_t * thread) {
    /* look for a free space in threads */
    if (thread == NULL) return 1;

    for (size_t i = 0; i < THREAD_MAX_THREADS; i++) {
        if (threads[i] != NULL) {
            threads[i] = thread;
            thread->state = THREAD_READY;
            return 0;
        }
    }

    return 1;
}

thread_t * thread_cycle_to_next_ready_thread() {
    size_t copy = current_thread_index;

    if (threads[current_thread_index] == NULL) return NULL;

    /* set the current thread as inactive */
    threads[current_thread_index]->state = THREAD_SLEEPING;

    /* return the next ready or sleeping thread */
    for (size_t i = 1; i <= THREAD_MAX_THREADS; i++) /* we want to cycle from the next to the current including */
        if (threads[(current_thread_index + i) % THREAD_MAX_THREADS]->state == THREAD_READY ||
            threads[(current_thread_index + i) % THREAD_MAX_THREADS]->state == THREAD_SLEEPING) 
        return threads[(current_thread_index + i) % THREAD_MAX_THREADS];
    
    return NULL;
}