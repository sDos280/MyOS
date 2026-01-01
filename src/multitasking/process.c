#include "multitasking/process.h"
#include "mm/kheap.h"
#include "mm/paging.h"
#include "kernel/print.h"
#include "utils.h"

static pid_t next_pid = 0;
process_t * process_list = NULL;  // process list head
process_t * current_process = NULL;

void process_init() {
}

process_t * process_create(void (*entry)(void *), size_t stack_size) {
    process_t * process = kalloc(sizeof(process_t));

    if (process == NULL) return NULL; /* no more memory or error */
    
    memset(process, 0, sizeof(process_t));

    process->pid = next_pid++;
    process->status = PROCESS_READY;
    process->context.ss = 0x10;  // Data segment
    process->context.esp = kalloc(0x100000); 
    process->context.eflags = 0x202;  // 0x0002 | 0x0200 <=> 0x0002 always on, 0x0200 interrupts enabled
    process->context.cs = 0x08;  // Code segment
    process->context.eip = (uint32_t)entry;  // Main entry
    process->context.edi = 0;  // Note: not really sure why set that, may not really need that
    process->context.ebp = 0;  // Zero is a special value that indicates we have reached the top-most stack frame

    return process;
}

uint8_t process_announce(process_t * process) {
    /* look for a free space in process_list */
    if (process == NULL) return 1;

    if (process_list == NULL) process_list = process;

    process_t * last = process_list;
    
    while (last->next != NULL) last = last->next;

    last->next = process;

    return 0;
}