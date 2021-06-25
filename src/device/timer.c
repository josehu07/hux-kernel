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


/**
 * Timer interrupt handler registered for IRQ # 0.
 * Currently just prints a tick message.
 */
static void
timer_interrupt_handler(interrupt_state_t *state)
{
    (void) state;   /** Unused. */
    // printf(".");
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
}
