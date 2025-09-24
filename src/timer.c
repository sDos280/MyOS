#include "timer.h"
#include "screen.h"
#include "port.h"

static uint32_t used_frequency = 0;
static uint32_t base_frequency = 1193180; // The PIT runs at 1.19318 MHz

void timer_interrupt_handler(registers_t* regs){
    static uint32_t tick = 0;
    tick++;
    if (tick % used_frequency == 0) { // Every second
        print("Tick: ");
        print_int(tick / used_frequency);
        print("\n");
    }
}

void initialize_timer(uint32_t frequency){
    used_frequency = frequency;
    uint32_t divisor = base_frequency / frequency;

    // Send the command byte.
    outb(TIMER_COMMAND_PORT, TIMER_CHANNEL_0 | TIMER_RATE_GENERATOR_MODE | TIMER_BINARY_MODE); // Command port
    outb(TIMER_DATA_0_PORT, divisor & 0xFF); // Low byte
    outb(TIMER_DATA_0_PORT, (divisor >> 8) & 0xFF); // High byte
    
    register_interrupt_handler(32, timer_interrupt_handler);
}