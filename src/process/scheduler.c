/**
 * CPU scheduler and context switching routines.
 *
 * Hux only aims to support a single CPU.
 */


#include <stddef.h>

#include "scheduler.h"
#include "process.h"

#include "../common/debug.h"


/** Global CPU state (only a single CPU). */
static cpu_state_t cpu_state;


/** Extern our context switch routine from ASM `switch.s`. */
extern void context_switch(process_context_t **old, process_context_t *new);


/** CPU scheduler, never leaves this function. */
void
scheduler(void)
{
    cpu_state.running_proc = 0;

    while (1) {     /** Loop indefinitely. */
        /** Look for a ready process in ptable. */
        process_t *proc;
        for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
            if (proc->state != READY)
                continue;

            info("scheduler: going to context switch to '%s'", proc->name);

            /**
             * Switch to chosen process.
             *
             * This is temprorary so we are not switching to user mode;
             * just doing a context switch of registers.
             */
            cpu_state.running_proc = proc;
            context_switch(&(cpu_state.scheduler), proc->context);
            /** Our demo init process never switches back... */

            cpu_state.running_proc = NULL;
        }
    }
}

// void
// scheduler(void)
// {
//     struct proc *p;
//     struct cpu *c = mycpu();
//     c->proc = 0;
    
//     for(;;){
//         // Enable interrupts on this processor.
//         sti();

//         // Loop over process table looking for process to run.
//         acquire(&ptable.lock);
//         for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//             if(p->state != RUNNABLE)
//                 continue;

//             // Switch to chosen process.  It is the process's job
//             // to release ptable.lock and then reacquire it
//             // before jumping back to us.
//             c->proc = p;
//             switchuvm(p);
//             p->state = RUNNING;

//             swtch(&(c->scheduler), p->context);
//             switchkvm();

//             // Process is done running for now.
//             // It should have changed its p->state before coming back.
//             c->proc = 0;
//         }
//         release(&ptable.lock);

//     }
// }


/** Initialize CPU state. */
void
cpu_init(void)
{
    cpu_state.scheduler = NULL;     /** Will be set at context switch. */
    cpu_state.running_proc = NULL;
}
