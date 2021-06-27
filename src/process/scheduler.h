/**
 * CPU scheduler and context switching routines.
 *
 * Hux only aims to support a single CPU.
 */


#ifndef SCHEDULER_H
#define SCHEDULER_H


#include "process.h"

#include "../interrupt/syscall.h"


/** Per-CPU state (we only have a single CPU). */
struct cpu_state {
    /** No ID field because only supporting single CPU. */
    process_context_t *scheduler;   /** CPU scheduler context. */
    process_t *running_proc;        /** The process running or NULL. */
    tss_t task_state;               /** Current process task state. */
};
typedef struct cpu_state cpu_state_t;


void cpu_init();

void scheduler();

process_t *running_proc();

void yield_to_scheduler(void);


#endif
