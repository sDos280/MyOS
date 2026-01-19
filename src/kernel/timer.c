#include "kernel/timer.h"
#include "io/port.h"
#include "io/pic.h"
#include "multitasking/scheduler.h"

/* =========================================================
                    CONSTANT DEFINITIONS
   ========================================================= */

/*
 * Number of nanoseconds in one second.
 * Used for sub-second timekeeping.
 */
#define NSEC_PER_SEC 1000000000U

/*
 * Base frequency of the PIT hardware (1.19318 MHz).
 */
static const uint32_t pit_base_frequency = 1193180;


/* =========================================================
                  TIMER STATE VARIABLES
   ========================================================= */

/*
 * Configured timer interrupt frequency (Hz).
 * Example: 1000 Hz → 1 interrupt every 1 ms.
 */
static uint32_t timer_hz = 1000;

/*
 * Nanoseconds added per timer interrupt.
 * Computed as NSEC_PER_SEC / timer_hz.
 */
static uint32_t ns_per_tick;

/*
 * Monotonic uptime counters.
 *
 * seconds      → full seconds since boot
 * nanoseconds  → sub-second remainder (< 1 second)
 *
 * This split avoids 64-bit arithmetic while
 * remaining drift-free.
 */
static uint32_t seconds = 0;
static uint32_t nanoseconds = 0;

/*
 * Raw interrupt tick counter.
 * Used for scheduler time slicing.
 */
static uint32_t tick = 0;


/* =========================================================
                  TIME QUERY FUNCTIONS
   ========================================================= */

/**
 * Returns milliseconds since boot.
 *
 * NOTE:
 *  - This will overflow after ~49 days.
 *  - For long-running systems, use seconds + sub-second.
 */
uint32_t timer_time_ms() {
    return seconds * 1000 + nanoseconds / 1000000;
}

/**
 * Returns seconds since boot.
 * Safe for long uptimes.
 */
uint32_t timer_time_seconds() {
    return seconds;
}


/* =========================================================
                 TIMER INTERRUPT HANDLER
   ========================================================= */

/**
 * Function: timer_interrupt_handler
 * -------------------------------------
 * Handles PIT IRQ0 interrupts.
 *
 * Responsibilities:
 *  - Update monotonic time counters
 *  - Drive scheduler preemption
 *  - Send End-Of-Interrupt (EOI) to PIC
 */
void timer_interrupt_handler(cpu_status_t *regs) {
    /* Count raw timer ticks */
    tick++;

    /* Advance sub-second time */
    nanoseconds += ns_per_tick;

    /* Carry into seconds if needed */
    if (nanoseconds >= NSEC_PER_SEC) {
        nanoseconds -= NSEC_PER_SEC;
        seconds++;
    }

    /*
     * Send EOI early.
     * If the scheduler switches tasks, the pic send EOI
     * in the isr handler may never be called
     * Fix: a better design would probably be to context switch in the 
     * isr sheduler it self
     */
    pic_sendEOI(32);

    /*
     * Scheduler tick:
     * Trigger a context switch once per second
     * worth of ticks (simple round-robin).
     */
    if (tick % timer_hz == 0) {
        scheduler_schedule();
    }
}


/* =========================================================
                    TIMER INITIALIZATION
   ========================================================= */

/**
 * Function: timer_init
 * -------------------------------------
 * Programs the PIT and enables timer interrupts.
 *
 * @frequency Desired interrupt frequency in Hz.
 */
void timer_init(uint32_t frequency) {
    timer_hz = frequency;
    tick = 0;

    /*
     * Compute nanoseconds per tick.
     * Integer division is safe here; remainder
     * error is bounded and does not accumulate.
     */
    ns_per_tick = NSEC_PER_SEC / timer_hz;

    /*
     * PIT divisor formula:
     *   divisor = base_frequency / desired_frequency
     */
    uint32_t divisor = pit_base_frequency / frequency;

    /*
     * Configure PIT:
     *  - Channel 0
     *  - Rate generator mode
     *  - Binary counting
     */
    outb(TIMER_COMMAND_PORT,
         TIMER_CHANNEL_0 |
         TIMER_RATE_GENERATOR_MODE |
         TIMER_BINARY_MODE);

    /* Send divisor (low byte first, then high byte) */
    outb(TIMER_DATA_0_PORT, divisor & 0xFF);
    outb(TIMER_DATA_0_PORT, (divisor >> 8) & 0xFF);

    /* Register IRQ0 handler (mapped to interrupt 32) */
    register_interrupt_handler(32, timer_interrupt_handler);
}
