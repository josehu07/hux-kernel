/**
 * Providing the abstraction of processes.
 */


#ifndef PROCESS_H
#define PROCESS_H


#include <stdint.h>

#include "../interrupt/isr.h"

#include "../memory/paging.h"


/** Max number of processes at any time. */
#define MAX_PROCS 32

/** Each process has a kernel stack of one page. */
#define KSTACK_SIZE PAGE_SIZE


/**
 * Process context registers defined to be saved across switches.
 *
 * PC is the last member (so the last value on stack not being explicitly
 * popped in `context_switch()), because it will then be used as the
 * return address of the snippet.
 */
struct process_context {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;   /** Frame pointer. */
    uint32_t eip;   /** Instruction pointer (PC). */
};
typedef struct process_context process_context_t;

/** Process state. */
enum process_state {
    UNUSED,     /** Indicates PCB slot unused. */
    INITIAL,
    READY,
    RUNNING,
    BLOCKED_ON_SLEEP,
    BLOCKED_ON_WAIT,
    TERMINATED
};
typedef enum process_state process_state_t;


/** Process control block (PCB). */
struct process {
    char name[16];                  /** Process name. */
    int8_t pid;                     /** Process ID. */
    process_context_t *context;     /** Registers context. */
    process_state_t state;          /** Process state */
    pde_t *pgdir;                   /** Process page directory. */
    uint32_t kstack;                /** Bottom of its kernel stack. */
    interrupt_state_t *trap_state;  /** Trap state of latest trap. */
    uint32_t stack_low;             /** Current bottom of stack pages. */
    uint32_t heap_high;             /** Current top of heap pages. */
    struct process *parent;         /** Parent process. */
    uint32_t target_tick;           /** Target wake up timer tick. */
    bool killed;                    /** True if should exit. */
    // ... (TODO)
};
typedef struct process process_t;


/** Extern the process table to the scheduler. */
extern process_t ptable[];
extern process_t *initproc;


void process_init();
void initproc_init();

int8_t process_fork();

void process_exit();

void process_sleep(uint32_t sleep_ticks);

int8_t process_wait();

int8_t process_kill(int8_t pid);


#endif
