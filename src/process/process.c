/**
 * Providing the abstraction of processes.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "process.h"
#include "layout.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../interrupt/isr.h"

#include "../memory/gdt.h"
#include "../memory/slabs.h"


/** Process table - list of PCB slots. */
process_t ptable[MAX_PROCS];

/** Next available PID value, incrementing overtime. */
static int8_t next_pid = 1;


/** Extern the `return_from_trap` snippet from ASM `isr-stub.s`. */
extern void return_from_trap(void);


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
    proc->pid = next_pid++;

    /** Allocate kernel stack. */
    proc->kstack = salloc_page();
    uint32_t sp = proc->kstack + KSTACK_SIZE;

    /**
     * Leave room for the trap state. The initial context will be pushed
     * right below this trap state, with return address EIP pointing to
     * `trapret` (the return-from-trap part of `isr_handler_stub`). In this
     * way, the new process, after context switched to by the scheduler,
     * automatically jumps into user mode execution. 
     */
    sp -= sizeof(interrupt_state_t);
    proc->trap_state = (interrupt_state_t *) sp;
    memset(proc->trap_state, 0, sizeof(interrupt_state_t));

    sp -= sizeof(process_context_t);
    proc->context = (process_context_t *) sp;
    memset(proc->context, 0, sizeof(process_context_t));
    proc->context->eip = (uint32_t) return_from_trap;

    return proc;
}


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
    extern char _binary___src_process_init_end[];
    char *elf_curr = (char *) _binary___src_process_init_start;
    char *elf_end  = (char *) _binary___src_process_init_end;

    /** Get a slot in the ptable. */
    process_t *proc = _alloc_new_process();
    assert(proc != NULL);
    strncpy(proc->name, "init", sizeof(proc->name) - 1);

    /**
     * Set up page tables and pre-map necessary pages:
     *   - kernel mapped to lower 512MiB
     *   - program ELF binary follows
     *   - top-most stack page
     */
    proc->pgdir = (pde_t *) salloc_page();
    memset(proc->pgdir, 0, sizeof(pde_t) * PDES_PER_PAGE);

    uint32_t vaddr_btm = 0;                     /** Kernel-mapped. */
    while (vaddr_btm < KMEM_MAX) {
        pte_t *pte = paging_walk_pgdir(proc->pgdir, vaddr_btm, true, false);
        paging_map_kpage(pte, vaddr_btm);

        vaddr_btm += PAGE_SIZE;
    }
    
    uint32_t vaddr_elf = USER_BASE;             /** ELF binary. */
    while (elf_curr < elf_end) {
        pte_t *pte = paging_walk_pgdir(proc->pgdir, vaddr_elf, true, false);
        uint32_t paddr = paging_map_upage(pte, true);
        
        /** Copy ELF content in. */
        memcpy((char *) paddr, elf_curr,
            elf_curr + PAGE_SIZE > elf_end ? elf_end - elf_curr : PAGE_SIZE);

        elf_curr += PAGE_SIZE;
    }
    
    uint32_t vaddr_top = USER_MAX - PAGE_SIZE;  /** Top stack page. */
    pte_t *pte_top = paging_walk_pgdir(proc->pgdir, vaddr_top, true, false);
    uint32_t paddr_top = paging_map_upage(pte_top, true);
    memset((char *) paddr_top, 0, PAGE_SIZE);

    /** Set up the trap state for returning to user mode. */
    proc->trap_state->cs = (8 * SEGMENT_UCODE) | 0x3;   /** DPL_USER. */
    proc->trap_state->ds = (8 * SEGMENT_UDATA) | 0x3;   /** DPL_USER. */
    proc->trap_state->ss = proc->trap_state->ds;
    proc->trap_state->eflags = 0x00000200;      /** Interrupt enable. */
    proc->trap_state->esp = USER_MAX;
    proc->trap_state->eip = USER_BASE;   /** Beginning of ELF binary. */

    /** Set process state to READY so the scheduler can pick it up. */
    proc->state = READY;
}
