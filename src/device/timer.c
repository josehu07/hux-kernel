/**
 * Programmable interval timer (PIT) in square wave generator mode to
 * serve as the system timer.
 */


#include <stdint.h>

#include "timer.h"


#include "../common/port.h"
#include "../common/printf.h"
#include "../common/debug.h"
#include "../common/spinlock.h"

#include "../interrupt/isr.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** Global counter of timer ticks elapsed since boot. */
uint32_t timer_tick = 0;
spinlock_t timer_tick_lock;


/**
 * Timer interrupt handler registered for IRQ # 0.
 * Mostly borrowed from xv6's `trap()` code. Interrupts should have been
 * disabled automatically since this is an interrupt gate.
 */
static void
timer_interrupt_handler(interrupt_state_t *state)
{
    (void) state;   /** Unused. */
    
    spinlock_acquire(&timer_tick_lock);

    /** Increment global timer tick. */
    timer_tick++;
    // printf(".");

    /**
     * Check all sleeping processes, set them back to READY if desired
     * ticks have passed.
     */
    spinlock_acquire(&ptable_lock);
    for (process_t *proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->state == BLOCKED && proc->block_on == ON_SLEEP
            && timer_tick >= proc->target_tick) {
            proc->target_tick = 0;
            process_unblock(proc);
        }
    }
    spinlock_release(&ptable_lock);

    spinlock_release(&timer_tick_lock);

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
     * If we are in a process and the process is in RUNNING state, yield
     * to the scheduler to force a new scheduling decision. Could happen
     * to a provess in kernel context (during a syscall) as well.
     */
    if (proc != NULL && proc->state == RUNNING) {
        spinlock_acquire(&ptable_lock);
        proc->state = READY;
        yield_to_scheduler();
        spinlock_release(&ptable_lock);
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
    timer_tick = 0;

    spinlock_init(&timer_tick_lock, "timer_tick_lock");

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
}
