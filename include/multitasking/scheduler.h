#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/description_tables.h"
#include "multitasking/process.h"

extern void scheduler_context_switch_asm(uint32_t ** current_stack_pointer, uint32_t * next_stack); /* calls sti!!!! */
extern void scheduler_start_thread_asm(uint32_t * thread_stack);

void scheduler_init();
void scheduler_add_process_to_ready_queue(process_t * process);
process_t * scheduler_get_next_process();
void scheduler_schedule();

#endif // SCHEDULER_H