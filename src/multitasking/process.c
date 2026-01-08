#include "multitasking/process.h"
#include "mm/kheap.h"
#include "mm/paging.h"
#include "kernel/print.h"
#include "utils.h"

static pid_t next_pid = 0;
process_t * processes_list = NULL;  // process list head
process_t * current_process = NULL;

static const char *process_state_str(process_state_e state)
{
    switch (state) {
        case PROCESS_READY:   return "READY";
        case PROCESS_RUNNING: return "RUNNING";
        case PROCESS_BLOCKED: return "BLOCKED";
        case PROCESS_ZOMBIE:  return "ZOMBIE";
        default:              return "UNKNOWN";
    }
}

void print_process_list(process_t *head)
{
    process_t *p = head;

    printf(" PID      STATE        EIP         ESP          NEXT\n");
    printf("---------------------------------------------------------\n");

    while (p) {
        printf("pid=%d state=%s eip=%p esp=%p next=%p\n",
                p->pid,
                process_state_str(p->status),
                (void *)p->context.eip,
                (void *)p->context.esp,
                (void *)p->next);

        p = p->next;
    }
}

void process_init() {
    processes_list = NULL;
}

process_t * process_create(void (*entry)(void *), size_t stack_size) {
    process_t * process = kalloc(sizeof(process_t));

    if (process == NULL) return NULL; /* no more memory or error */
    
    memset(process, 0, sizeof(process_t));

    process->pid = next_pid++;
    process->status = PROCESS_READY;
    process->context.ds = 0x10;  // Data segment
    process->context.esp = kalloc(stack_size) + stack_size; 
    process->context.eflags = 0x202;  // 0x0002 | 0x0200 <=> 0x0002 always on, 0x0200 interrupts enabled
    process->context.cs = 0x08;  // Code segment
    process->context.eip = (uint32_t)entry;  // Main entry
    process->context.edi = 0;  // Note: not really sure why set that, may not really need that
    process->context.ebp = 0;  // Zero is a special value that indicates we have reached the top-most stack frame
    process->next = NULL;

    return process;
}

uint8_t process_announce(process_t * process) {
    /* look for a free space in process_list */
    if (process == NULL) return 1;

    if (processes_list  == NULL) {
        processes_list = process;
        return 0;
    }

    process_t *last = processes_list;
    while (last->next)
        last = last->next;

    last->next = process;
    return 0;
}

uint8_t process_set_current(process_t * process) {
    if (current_process == NULL) {
        current_process = process;
        return 0;
    }
    
    current_process = process;
}