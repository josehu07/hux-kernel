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

        spinlock_acquire(&ptable_lock);

        /** Look for a ready process in ptable. */
        process_t *proc;
        for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
            if (proc->state != READY)
                continue;

            /** Schedule this one for at most its `timeslice` ticks. */
            uint32_t next_sched_tick = timer_tick + proc->timeslice;
            while (timer_tick < next_sched_tick && proc->state == READY) {
                // info("scheduler: going to context switch to %d - '%s'",
                //      proc->pid, proc->name);

                /** Set up TSS for this process, and switch page directory. */
                cli_push();
                gdt_switch_tss(&(cpu_state.task_state), proc);
                paging_switch_pgdir(proc->pgdir);
                cli_pop();
                
                cpu_state.running_proc = proc;
                proc->state = RUNNING;

                /**
                 * Force `int_enabled` to be true, though this is not
                 * necessary because the scheduled process must do `iret`
                 * before going back to user space, and that pops an
                 * EFLAGS register value with interrupt enable flag on.
                 */
                cpu_state.int_enabled = true;

                /** Do the context switch. */
                context_switch(&(cpu_state.scheduler), proc->context);

                /** It switches back, switch to kernel page directory. */
                paging_switch_pgdir(kernel_pgdir);
                cpu_state.running_proc = NULL;
            }
        }

        spinlock_release(&ptable_lock);
    }
}


/** Get the current scheduled process. */
inline process_t *
running_proc(void)
{
    /** Need to disable interrupts since the get might not be atomic. */
    cli_push();
    process_t *proc = cpu_state.running_proc;
    cli_pop();

    return proc;
}


/**
 * Return back to the scheduler context.
 * Must be called with `ptable_lock` held.
 */
void
yield_to_scheduler(void)
{
    process_t *proc = running_proc();
    assert(proc->state != RUNNING);
    assert(!interrupt_enabled());
    assert(cpu_state.cli_depth == 1);
    assert(spinlock_locked(&ptable_lock));
    
    /**
     * Save & restore int_enable state because this state is essentially
     * a per-process state instead of a pre-CPU state. Imagine the case
     * where process A forks child B and gets timer-interrupted, yields
     * to the scheduler (so cli stack depth is 1 with `int_enabled` being
     * false), and the scheduler picks B to run which starts with the
     * `_new_process_entry()` function in `process.c` that pops the held
     * cli. The return back to user mode (return-from-trap) will pop the
     * EFLAGS register stored on kernel stack of B, whose value should be
     * 0x202 that enables interrupts in user mode execution of B, which
     * is good. If B ever calls `cli_push()` e.g. in a syscall, the CPU
     * `int_enabled` will become overwritten as true. When it comes back
     * to A's turn, A's false `int_enabled` state gets lost. Hence, we
     * save & restore this state here to let it become somewhat process-
     * private.
     */
    bool int_enabled = cpu_state.int_enabled;
    context_switch(&(proc->context), cpu_state.scheduler);
    cpu_state.int_enabled = int_enabled;
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
