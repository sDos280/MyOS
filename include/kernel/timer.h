#ifndef TIMER_H
#define TIMER_H

#include "kernel/description_tables.h"
#include "types.h"

/* =========================================================
                       PIT I/O PORTS
   ========================================================= */

/*
 * The Programmable Interval Timer (PIT) has three data ports
 * and one command port.
 */
#define TIMER_DATA_0_PORT 0x40
#define TIMER_DATA_1_PORT 0x41
#define TIMER_DATA_2_PORT 0x42
#define TIMER_COMMAND_PORT 0x43


/* =========================================================
                     PIT COMMAND FLAGS
   ========================================================= */

/* Channel selection */
#define TIMER_CHANNEL_0       0b00000000
#define TIMER_CHANNEL_1       0b01000000
#define TIMER_CHANNEL_2       0b10000000
#define TIMER_READ_BACK_LATCH 0b11000000

/* Access / operating modes */
#define TIMER_BINARY_MODE       0b00000000
#define TIMER_BCD_MODE          0b00000001

#define TIMER_INTERRUPT_ON_TERMINAL_COUNT_MODE  0b00000000
#define TIMER_HARDWARE_RETRIGGERABLE_ONE_SHOT_MODE 0b00000010
#define TIMER_RATE_GENERATOR_MODE 0b00000100
#define TIMER_SQUARE_WAVE_GENERATOR_MODE 0b00000110

/* =========================================================
                      TIMER API
   ========================================================= */

/**
 * Returns milliseconds since system boot.
 * NOTE: Overflows after ~49 days on 32-bit systems.
 */
uint32_t timer_time_ms();

/**
 * Returns seconds since system boot.
 * Safe for long uptimes (~136 years).
 */
uint32_t timer_time_seconds();

/**
 * Main PIT interrupt handler (IRQ0).
 * Handles timekeeping and scheduler ticks.
 */
uint32_t timer_interrupt_handler(cpu_status_t *regs);

/**
 * Initializes the PIT and registers the IRQ handler.
 * @frequency Desired timer frequency (e.g. 1000 Hz).
 */
void timer_init(uint32_t frequency);

#endif // TIMER_H
