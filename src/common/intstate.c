/**
 * Interrupt enable/disable routines. Mimics xv6.
 */


#include <stdbool.h>

#include "intstate.h"
#include "debug.h"

#include "../process/scheduler.h"


/** Check if interrupts are enabled. */
inline bool
interrupt_enabled(void)
{
    uint32_t eflags;
    asm volatile ( "pushfl; popl %0" : "=r" (eflags) : );
    return eflags & 0x0200;     /** IF flag. */
}


/** Disable interrupts if not yet so. */
void
cli_push(void)
{
    bool was_enabled = interrupt_enabled();
    asm volatile ( "cli" );

    /**
     * If cli stack previously empty, remember the previous interrupt
     * enable/disable state.
     */
    if (cpu_state.cli_depth == 0)
        cpu_state.int_enabled = was_enabled;
    cpu_state.cli_depth++;
}

/**
 * Restore interrupt e/d state to previous state if all `cli`s have been
 * popped. Must be one-one mapped to `cli_push()` in code.
 */
void
cli_pop(void)
{
    assert(!interrupt_enabled());
    assert(cpu_state.cli_depth > 0);

    cpu_state.cli_depth--;
    if (cpu_state.cli_depth == 0 && cpu_state.int_enabled)
        asm volatile ( "sti" );
}
