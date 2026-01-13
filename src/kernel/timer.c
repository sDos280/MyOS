#include "kernel/timer.h"
#include "kernel/print.h"
#include "io/port.h"
#include "io/pic.h"
#include "multitasking/scheduler.h"
#include "utils.h"

static uint32_t used_frequency = 0;
static uint32_t base_frequency = 1193180; // The PIT runs at 1.19318 MHz
static uint32_t tick = 0;

void timer_interrupt_handler(cpu_timer_status_t* regs){
    tick++;
    pic_sendEOI(32); /* send a pic now, since after a shedule we wouldn't be able to sent it */

    if (tick % used_frequency == 0)
        schduler_schedule();
}

void timer_init(uint32_t frequency){
    tick = 0;
    used_frequency = frequency;
    uint32_t divisor = base_frequency / frequency;

    // Send the command byte.
    outb(TIMER_COMMAND_PORT, TIMER_CHANNEL_0 | TIMER_RATE_GENERATOR_MODE | TIMER_BINARY_MODE); // Command port
    outb(TIMER_DATA_0_PORT, divisor & 0xFF); // Low byte
    outb(TIMER_DATA_0_PORT, (divisor >> 8) & 0xFF); // High byte
    
    register_interrupt_handler(32, timer_interrupt_handler);
}