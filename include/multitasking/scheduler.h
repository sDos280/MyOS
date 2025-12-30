#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "multitasking/thread.h"

uint8_t scheduler_schedule_next();  /* constact switch to the next thread */

#endif // SCHEDULER_H