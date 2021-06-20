/**
 * CPU state structure. Hux only aims to support a single CPU.
 */


#ifndef CPU_H
#define CPU_H


#include "process.h"


/** Per-CPU state. */
struct cpu_state {
    /** No ID field because only supporting single CPU. */
    process_context_t *scheduler;   /** CPU scheduler context. */
    process_t *running_proc;        /** The process running or NULL. */
};
typedef struct cpu_state cpu_state_t;

// Per-CPU state
// struct cpu {
//   uchar apicid;                // Local APIC ID
//   struct context *scheduler;   // swtch() here to enter scheduler
//   struct taskstate ts;         // Used by x86 to find stack for interrupt
//   struct segdesc gdt[NSEGS];   // x86 global descriptor table
//   volatile uint started;       // Has the CPU started?
//   int ncli;                    // Depth of pushcli nesting.
//   int intena;                  // Were interrupts enabled before pushcli?
//   struct proc *proc;           // The process running on this cpu or null
// };


/**
 * CPU global state externed.
 * Just a single struct (not an array) because Hux only aims to support
 * a single CPU.
 */
extern cpu_state_t cpu_state;


#endif
