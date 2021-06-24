/**
 * Providing the abstraction of processes.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "process.h"

#include "../common/string.h"

#include "../memory/slabs.h"


/** Process table - list of PCB slots. */
process_t ptable[MAX_PROCS];

/** Next available PID value, incrementing overtime. */
static int8_t next_pid = 1;


/**
 * Find an UNUSED slot in the ptable and put it into INITIAL state. If
 * all slots are in use, return NULL.
 */
static process_t *
_alloc_new_process(void)
{
    process_t *proc;
    bool found = false;

    for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->state == UNUSED) {
            found = true;
            break;
        }
    }

    if (!found)
        return NULL;

    /** Make proper setups for the new process. */
    proc->state = INITIAL;
    proc->pid = next_pid;

    /** Allocate kernel stack. */
    proc->kstack = salloc_page();

    return proc;
}

// static struct proc*
// allocproc(void)
// {
//   struct proc *p;
//   char *sp;

//   acquire(&ptable.lock);

//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     if(p->state == UNUSED)
//       goto found;

//   release(&ptable.lock);
//   return 0;

// found:
//   p->state = EMBRYO;
//   p->pid = nextpid++;

//   release(&ptable.lock);

//   // Allocate kernel stack.
//   if((p->kstack = kalloc()) == 0){
//     p->state = UNUSED;
//     return 0;
//   }
//   sp = p->kstack + KSTACKSIZE;

//   // Leave room for trap frame.
//   sp -= sizeof *p->tf;
//   p->tf = (struct trapframe*)sp;

//   // Set up new context to start executing at forkret,
//   // which returns to trapret.
//   sp -= 4;
//   *(uint*)sp = (uint)trapret;

//   sp -= sizeof *p->context;
//   p->context = (struct context*)sp;
//   memset(p->context, 0, sizeof *p->context);
//   p->context->eip = (uint)forkret;

//   return p;
// }


/** Fill the ptable with UNUSED entries. */
void
process_init(void)
{
    for (size_t i = 0; i < MAX_PROCS; ++i)
        ptable[i].state = UNUSED;

    next_pid = 1;
}

/**
 * Initialize the `init` process - put it in READY state in the process
 * table so the scheduler can pick it up.
 */
void
initproc_init(void)
{
    /** Get the embedded binary of `init.s`. */
    extern char _binary___src_process_init_start[];
    extern char _binary___src_process_init_size[];  // Not used for now.

    /** Get a slot in the ptable. */
    process_t *proc = _alloc_new_process();
    strncpy(proc->name, "init", sizeof(proc->name) - 1);

    /**
     * Set up context, set EIP to be the start of its binary code.
     * 
     * This is temporary because we are in the process's kernel stack.
     * After user mode execution has been explained, it should be changed
     * to a "return-from-trap" context. User page tables must also be
     * set up, and switched properly in the `scheduler()` function.
     */
    uint32_t sp = proc->kstack + KSTACK_SIZE;
    sp -= sizeof(process_context_t);
    
    proc->context = (process_context_t *) sp;
    memset(proc->context, 0, sizeof(process_context_t));
    // proc->context->eip = (uint32_t) _binary_init_start;
    proc->context->eip = (uint32_t) _binary___src_process_init_start;

    /** Set process state to READY so the scheduler can pick it up. */
    proc->state = READY;
}

// void
// userinit(void)
// {
//   struct proc *p;
//   extern char _binary_initcode_start[], _binary_initcode_size[];

//   p = allocproc();
  
//   initproc = p;
//   if((p->pgdir = setupkvm()) == 0)
//     panic("userinit: out of memory?");
//   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
//   p->sz = PGSIZE;
//   memset(p->tf, 0, sizeof(*p->tf));
//   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
//   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
//   p->tf->es = p->tf->ds;
//   p->tf->ss = p->tf->ds;
//   p->tf->eflags = FL_IF;
//   p->tf->esp = PGSIZE;
//   p->tf->eip = 0;  // beginning of initcode.S

//   safestrcpy(p->name, "initcode", sizeof(p->name));
//   p->cwd = namei("/");

//   // this assignment to p->state lets other cores
//   // run this process. the acquire forces the above
//   // writes to be visible, and the lock is also needed
//   // because the assignment might not be atomic.
//   acquire(&ptable.lock);

//   p->state = RUNNABLE;

//   release(&ptable.lock);
// }

// void
// inituvm(pde_t *pgdir, char *init, uint sz)
// {
//   char *mem;

//   if(sz >= PGSIZE)
//     panic("inituvm: more than a page");
//   mem = kalloc();
//   memset(mem, 0, PGSIZE);
//   mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
//   memmove(mem, init, sz);
// }
