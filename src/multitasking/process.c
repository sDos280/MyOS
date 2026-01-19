#include "multitasking/process.h"
#include "mm/kheap.h"
#include "mm/paging.h"
#include "kernel/print.h"
#include "utils.h"

static pid_t next_pid = 0;

static const char *process_state_str(process_state_e state) {
    switch (state) {
        case PROCESS_NEW:     return "NEW";
        case PROCESS_READY:   return "READY";
        case PROCESS_RUNNING: return "RUNNING";
        case PROCESS_BLOCKED: return "BLOCKED";
        case PROCESS_ZOMBIE:  return "ZOMBIE";
        default:              return "UNKNOWN";
    }
}

void print_process_list(process_t *head) {
    process_t *p = head;

    printf(" PID      STATE            ESP          NEXT\n");
    printf("---------------------------------------------------------\n");

    while (p) {
        printf("pid=%d state=%s esp=%p next=%p\n",
                p->pid,
                process_state_str(p->status),
                (void *)p->esp,
                (void *)p->next);

        p = p->next;
    }
}

process_t * process_create(process_type_e type, void (*entry)(void), size_t stack_size) {
    void scheduler_thread_exit();

    process_t * process = kalloc(sizeof(process_t));

    if (process == NULL) return NULL; /* no more memory or error */
    
    memset(process, 0, sizeof(process_t));

    process->pid = next_pid++;
    process->status = PROCESS_NEW;
    process->type = type;
    process->esp = (uint32_t *)((uint32_t)kalloc(stack_size) + (stack_size - 1));
    /* Since we don't don't *call* entry we just use *ret* when entry is in the stack top, 
       the *ret* in the entry function would pop sheudler_thread_exit and redirect code to there */
    *(--process->esp) = (uint32_t)scheduler_thread_exit;
    *(--process->esp) = (uint32_t)entry;  /* Main Entry */
    *(--process->esp) = 0;      /* edi */
    *(--process->esp) = 0;      /* esi */
    *(--process->esp) = 0;      /* ebp */
    *(--process->esp) = 0;      /* esp, ignored by popa */
    *(--process->esp) = 0;      /* ebx */
    *(--process->esp) = 0;      /* edx */
    *(--process->esp) = 0;      /* ecx */
    *(--process->esp) = 0;  /* eax */
    
    process->next = NULL;

    return process;
}