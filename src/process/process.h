/**
 * Providing the abstraction of processes.
 */


#ifndef PROCESS_H
#define PROCESS_H


#include <stdint.h>


/** Max number of processes at any time. */
#define MAX_PROCS 128


/** Process context registers defined to be saved across switches. */
// struct process_context {
//     uint32_t edi;
//     uint32_t esi;
//     uint32_t ebx;
//     uint32_t ebp;   /** Frame pointer. */
//     uint32_t eip;   /** Instruction pointer (PC). */
//     // Maybe kernel stack goes here as well?
// };
// typedef struct process_context process_context_t;


/** Process state. */
// enum process_state {
//     UNUSED,     /** Indicates PCB slot unused. */
//     INITIAL,
//     READY,
//     RUNNING,
//     BLOCKED,
//     TERMINATED
// };
// typedef enum process_state process_state_t;

// struct context {
//   uint edi;
//   uint esi;
//   uint ebx;
//   uint ebp;
//   uint eip;
// };

// enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };


/** Process control block. */
// struct process {
//     char name[16];                  /** Process name. */
//     int pid;                        /** Process ID. */
//     process_context_t *context;     /** Registers context. */
//     process_state_t state;          /** Process state */
//     // ...
// }

// Per-process state
// struct proc {
//   uint sz;                     // Size of process memory (bytes)
//   pde_t* pgdir;                // Page table
//   char *kstack;                // Bottom of kernel stack for this process
//   enum procstate state;        // Process state
//   int pid;                     // Process ID
//   struct proc *parent;         // Parent process
//   struct trapframe *tf;        // Trap frame for current syscall
//   struct context *context;     // swtch() here to run process
//   void *chan;                  // If non-zero, sleeping on chan
//   int killed;                  // If non-zero, have been killed
//   struct file *ofile[NOFILE];  // Open files
//   struct inode *cwd;           // Current directory
//   char name[16];               // Process name (debugging)
// };


/** Process table externed. */
// extern process_t ptable[];


#endif
