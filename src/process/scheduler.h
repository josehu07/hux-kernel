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
    bool int_enabled;               /** Remembered interrupt e/d state. */
    uint8_t cli_depth;              /** Number of pushed `cli`s. */
};
typedef struct cpu_state cpu_state_t;


/** Extern the CPU state to interrupt enable/disable state helpers. */
extern cpu_state_t cpu_state;

process_t *running_proc();


void cpu_init();

void scheduler();

void yield_to_scheduler(void);


#endif
