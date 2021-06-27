/**
 * Programmable interval timer (PIT) in square wave generator mode to
 * serve as the system timer.
 */


#include <stdint.h>

#include "timer.h"


#include "../common/port.h"
#include "../common/printf.h"
#include "../common/debug.h"

#include "../interrupt/isr.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** Global counter of timer ticks elapsed since boot. */
uint32_t timer_tick = 0;


/**
 * Timer interrupt handler registered for IRQ # 0.
 * Currently just prints a tick message.
 */
static void
timer_interrupt_handler(interrupt_state_t *state)
{
    (void) state;   /** Unused. */
    
    /** Increment global timer tick. */
    timer_tick++;

    /**
     * Check all sleeping processes, set them back to READY if desired
     * ticks have passed.
     */
    for (process_t *proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->state == BLOCKED_ON_SLEEP && timer_tick >= proc->target_tick)
            proc->state = READY;
    }

    process_t *proc = running_proc();
    bool user_context = (state->cs & 0x3) == 3      /** DPL field is 3. */
                        && proc != NULL;

    /**
     * If we are now in the process's context and it is set to be killed,
     * exit the process right now.
     */
    if (user_context && proc->killed)
        process_exit();

    /**
     * If in was in user-space execution and the process is in RUNNING
     * state, yield to the scheduler to force a new scheduling decision.
     */
    if (proc != NULL && proc->state == RUNNING) {
        proc->state = READY;
        yield_to_scheduler();
    }

    /** Re-check if we get killed since the yield. */
    if (user_context && proc->killed)
        process_exit();
}


/**
 * Initialize the PIT timer. Registers timer interrupt ISR handler, sets
 * PIT to run in mode 3 with given frequency in Hz.
 */
void
timer_init(void)
{
    /** Register timer interrupt ISR handler. */
    isr_register(INT_NO_TIMER, &timer_interrupt_handler);

    /**
     * Calculate the frequency divisor needed to run with the given
     * frequency. Divisor = base frequencty / desired frequency.
     */
    uint16_t divisor = 1193182 / TIMER_FREQ_HZ;

    outb(0x43, 0x36);   /** Run in mode 3. */

    /** Sends frequency divisor, in lo | hi order. */
    outb(0x40, (uint8_t) (divisor & 0xFF));
    outb(0x40, (uint8_t) ((divisor >> 8) & 0xFF));

    timer_tick = 0;
}
