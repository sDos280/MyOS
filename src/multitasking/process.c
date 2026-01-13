#include "multitasking/process.h"
#include "mm/kheap.h"
#include "mm/paging.h"
#include "kernel/print.h"
#include "utils.h"

static pid_t next_pid = 0;

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

void print_process_list(process_t *head) {
    process_t *p = head;

    printf(" PID      STATE        EIP         ESP          NEXT\n");
    printf("---------------------------------------------------------\n");

    while (p) {
        printf("pid=%d state=%s eip=%p esp=%p next=%p\n",
                p->pid,
                process_state_str(p->status),
                (void *)p->next);

        p = p->next;
    }
}

process_t * process_create(void (*entry)(void), size_t stack_size) {
    process_t * process = kalloc(sizeof(process_t));

    if (process == NULL) return NULL; /* no more memory or error */
    
    memset(process, 0, sizeof(process_t));

    process->pid = next_pid++;
    process->status = PROCESS_RUNNING;
    process->esp = (uint32_t *)(kalloc(stack_size) + (stack_size /*- 1*/));
    /* Note the following 2 pushed may be redundant */
    *(--process->esp) = NULL;   /* &Current thread stack (shoudn't be poped) */
    *(--process->esp) = NULL;   /* Next Thread stack (shoudn't be poped) */
    /* Fix: there is a need to add an entry to some function like thread_exit */
    *(--process->esp) = entry;  /* Main Entry */
    uint32_t * temp = process->esp; /* there is a need to pop*/
    *(--process->esp) = 0;      /* edi */
    *(--process->esp) = 0;      /* esi */
    *(--process->esp) = 0;      /* ebp */
    *(--process->esp) = temp;   /* original esp */
    *(--process->esp) = 0;      /* ebx */
    *(--process->esp) = 0;      /* edx */
    *(--process->esp) = 0;      /* ecx */
    *(--process->esp) = 0;      /* eax */
    
    process->next = NULL;

    return process;
}