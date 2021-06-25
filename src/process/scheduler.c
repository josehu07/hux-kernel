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

#include "../memory/gdt.h"
#include "../memory/paging.h"


/** Global CPU state (only a single CPU). */
static cpu_state_t cpu_state;


/** Extern our context switch routine from ASM `switch.s`. */
extern void context_switch(process_context_t **old, process_context_t *new);


/** CPU scheduler, never leaves this function. */
void
scheduler(void)
{
    cpu_state.running_proc = NULL;

    while (1) {     /** Loop indefinitely. */
        /** Look for a ready process in ptable. */
        process_t *proc;
        for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
            if (proc->state != READY)
                continue;

            info("scheduler: going to context switch to '%s'", proc->name);

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
        }
    }
}


/** Initialize CPU state. */
void
cpu_init(void)
{
    cpu_state.scheduler = NULL;     /** Will be set at context switch. */
    cpu_state.running_proc = NULL;
    memset(&(cpu_state.task_state), 0, sizeof(tss_t));
}
