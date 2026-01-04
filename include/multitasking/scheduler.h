#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/description_tables.h"
#include "multitasking/process.h"

void sheduler_run();
cpu_status_t scheduler_schedule(cpu_status_t * context);  /* constact switch to the next process */

#endif // SCHEDULER_H