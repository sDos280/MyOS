#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/description_tables.h"
#include "multitasking/process.h"

extern void scheduler_switch_to_task_asm(uint32_t ** current_stack, uint32_t * next_stack);
extern void scheduler_start_first_thread_asm(uint32_t * thread_stack);
void scheduler_add_processes_to_list(process_t * process);
process_t * sheduler_get_current_process();
process_t * scheduler_get_next_process(); 
void schduler_schedule();

#endif // SCHEDULER_H