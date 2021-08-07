/**
 * CPU scheduler and context switching routines.
 *
 * Hux only aims to support a single CPU.
 */


#include <stddef.h>

#include "scheduler.h"
#include "process.h"

#include "../common/string.h"
#include "../common/debug.h"
#include "../common/intstate.h"

#include "../device/timer.h"

#include "../memory/gdt.h"
#include "../memory/paging.h"


/** Global CPU state (only a single CPU). */
cpu_state_t cpu_state;


/** Extern our context switch routine from ASM `switch.s`. */
extern void context_switch(process_context_t **old, process_context_t *new);


/** CPU scheduler, never leaves this function. */
void
scheduler(void)
{
    cpu_state.running_proc = NULL;

    while (1) {     /** Loop indefinitely. */
        /**
         * Force an interrupt enable in every iteration of this loop.
         * This primarily gives a chance to see keyboard input interrupts
         * and other similar external device interrupts, in case all live
         * processes are blocking (so the CPU looping in the scheduler).
         */
        asm volatile ( "sti" );

        cli_push();

        /** Look for a ready process in ptable. */
        process_t *proc;
        for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
            if (proc->state != READY)
                continue;

            /** Schedule this one for at most its `timeslice` ticks. */
            uint8_t ticks_left = proc->timeslice;
            while (ticks_left > 0) {
                // info("scheduler: going to context switch to %d - '%s'",
                //      proc->pid, proc->name);

                /** Set up TSS for this process, and switch page directory. */
                gdt_switch_tss(&(cpu_state.task_state), proc);
                paging_switch_pgdir(proc->pgdir);
                
                cpu_state.running_proc = proc;
                proc->state = RUNNING;

                /** Do the context switch. */
                context_switch(&(cpu_state.scheduler), proc->context);

                /** It switches back, switch to kernel page directory. */
                paging_switch_pgdir(kernel_pgdir);
                cpu_state.running_proc = NULL;

                /** Decrement its current share of ticks. */
                ticks_left--;
            }
        }

        cli_pop();
    }
}


/** Get the current scheduled process. */
inline process_t *
running_proc(void)
{
    cli_push();
    process_t *proc = cpu_state.running_proc;
    cli_pop();

    return proc;
}


/**
 * Return back to the scheduler context.
 * Must be called with `cli` pushed.
 */
void
yield_to_scheduler(void)
{
    process_t *proc = running_proc();
    assert(proc->state != RUNNING);
    assert(!interrupt_enabled());
    
    context_switch(&(proc->context), cpu_state.scheduler);
}


/** Initialize CPU state. */
void
cpu_init(void)
{
    cpu_state.scheduler = NULL;     /** Will be set at context switch. */
    cpu_state.running_proc = NULL;
    memset(&(cpu_state.task_state), 0, sizeof(tss_t));
    
    cpu_state.int_enabled = true;
    cpu_state.cli_depth = 0;
}
