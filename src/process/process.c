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
#include "../common/spinlock.h"

#include "../device/timer.h"

#include "../interrupt/isr.h"

#include "../memory/gdt.h"
#include "../memory/slabs.h"

#include "../filesys/file.h"


/** Process table - list of PCB slots. */
process_t ptable[MAX_PROCS];
spinlock_t ptable_lock;

/** Pointing to the `init` process. */
process_t *initproc;

/** Next available PID value, incrementing overtime. */
static int8_t next_pid = 1;


/** Extern the `return_from_trap` snippet from ASM `isr-stub.s`. */
extern void return_from_trap(void);


/**
 * Any new process "returns" to here, which in turn returns to the
 * return-from-trap part of `isr_handler_stub`, entering user mode
 * execution.
 */
static void
_new_process_entry(void)
{
    /** Release the lock held in the scheduler context. */
    spinlock_release(&ptable_lock);

    /**
     * The trap state on kernel stack, which will get popped, has EFLAGS
     * register value of 0x202 which means interrupts will be enabled once
     * enters user mode execution. See `yield_to_scheduler()` in
     * `scheduler.c` for more.
     */
}

/**
 * Find an UNUSED slot in the ptable and put it into INITIAL state. If
 * all slots are in use, return NULL.
 */
static process_t *
_alloc_new_process(void)
{
    process_t *proc;
    bool found = false;

    spinlock_acquire(&ptable_lock);

    for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->state == UNUSED) {
            found = true;
            break;
        }
    }

    if (!found) {
        warn("new_process: process table is full, no free slot");
        spinlock_release(&ptable_lock);
        return NULL;
    }

    /** Allocate kernel stack. */
    proc->kstack = salloc_page();
    if (proc->kstack == 0) {
        warn("new_process: failed to allocate kernel stack page");
        return NULL;
    }
    uint32_t sp = proc->kstack + KSTACK_SIZE;

    /** Make proper setups for the new process. */
    proc->state = INITIAL;
    proc->block_on = NOTHING;
    proc->pid = next_pid++;
    proc->target_tick = 0;
    proc->wait_req  = NULL;
    proc->wait_lock = NULL;
    for (size_t i = 0; i < MAX_FILES_PER_PROC; ++i)
        proc->files[i] = NULL;

    spinlock_release(&ptable_lock);

    /**
     * Leave room for the trap state. The initial context will be pushed
     * right below this trap state, with return address EIP pointing to
     * `_new_process_entry()` which returns to the return-from-trap part
     * of `isr_handler_stub`.
     * 
     * In this way, the new process, after context switched to by the
     * scheduler, automatically jumps into user mode execution.
     */
    sp -= sizeof(interrupt_state_t);
    proc->trap_state = (interrupt_state_t *) sp;
    memset(proc->trap_state, 0, sizeof(interrupt_state_t));

    sp -= sizeof(uint32_t);
    *(uint32_t *) sp = (uint32_t) return_from_trap;

    sp -= sizeof(process_context_t);
    proc->context = (process_context_t *) sp;
    memset(proc->context, 0, sizeof(process_context_t));
    proc->context->eip = (uint32_t) _new_process_entry;

    return proc;
}


/** Fill the ptable with UNUSED entries. */
void
process_init(void)
{
    spinlock_init(&ptable_lock, "ptable_lock");

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
    extern char _binary___user_init_start[];
    extern char _binary___user_init_end[];
    char *elf_curr = (char *) _binary___user_init_start;
    char *elf_end  = (char *) _binary___user_init_end;

    /** Get a slot in the ptable. */
    process_t *proc = _alloc_new_process();
    assert(proc != NULL);
    strncpy(proc->name, "init", sizeof(proc->name) - 1);
    proc->parent = NULL;

    /**
     * Set up page tables and pre-map necessary pages:
     *   - kernel mapped to lower 512MiB
     *   - program ELF binary follows
     *   - top-most stack page
     */
    proc->pgdir = (pde_t *) salloc_page();
    assert(proc->pgdir != NULL);
    memset(proc->pgdir, 0, sizeof(pde_t) * PDES_PER_PAGE);

    uint32_t vaddr_btm = 0;                     /** Kernel-mapped. */
    while (vaddr_btm < PHYS_MAX) {
        pte_t *pte = paging_walk_pgdir(proc->pgdir, vaddr_btm, true);
        assert(pte != NULL);
        paging_map_kpage(pte, vaddr_btm);

        vaddr_btm += PAGE_SIZE;
    }
    
    uint32_t vaddr_elf = USER_BASE;             /** ELF binary. */
    while (elf_curr < elf_end) {
        pte_t *pte = paging_walk_pgdir(proc->pgdir, vaddr_elf, true);
        assert(pte != NULL);
        uint32_t paddr = paging_map_upage(pte, true);
        assert(paddr != 0);

        /** Copy ELF content in. */
        memcpy((char *) paddr, elf_curr,
            elf_curr + PAGE_SIZE > elf_end ? elf_end - elf_curr : PAGE_SIZE);

        vaddr_elf += PAGE_SIZE;
        elf_curr += PAGE_SIZE;
    }

    while (vaddr_elf < HEAP_BASE) {             /** Rest of ELF region. */
        pte_t *pte = paging_walk_pgdir(proc->pgdir, vaddr_elf, true);
        assert(pte != NULL);
        uint32_t paddr = paging_map_upage(pte, true);
        assert(paddr != 0);

        vaddr_elf += PAGE_SIZE;
    }
    
    uint32_t vaddr_top = USER_MAX - PAGE_SIZE;  /** Top stack page. */
    pte_t *pte_top = paging_walk_pgdir(proc->pgdir, vaddr_top, true);
    assert(pte_top != NULL);
    uint32_t paddr_top = paging_map_upage(pte_top, true);
    assert(paddr_top != 0);
    memset((char *) paddr_top, 0, PAGE_SIZE);

    /** Set up the trap state for returning to user mode. */
    proc->trap_state->cs = (SEGMENT_UCODE << 3) | 0x3;  /** DPL_USER. */
    proc->trap_state->ds = (SEGMENT_UDATA << 3) | 0x3;  /** DPL_USER. */
    proc->trap_state->ss = proc->trap_state->ds;
    proc->trap_state->eflags = 0x00000202;      /** Interrupt enable. */
    proc->trap_state->esp = USER_MAX - 4;   /** GCC might push an FP. */
    proc->trap_state->eip = USER_BASE;   /** Beginning of ELF binary. */

    proc->stack_low = vaddr_top;
    proc->heap_high = HEAP_BASE;

    proc->timeslice = 1;

    /** Initially at root directory '/'. */
    proc->cwd = inode_get_at_boot(ROOT_INUMBER);
    if (proc->cwd == NULL)
        error("initproc_init: failed to get inode of root directory");

    /** Set process state to READY so the scheduler can pick it up. */
    initproc = proc;
    proc->killed = false;

    /** Needed to hold lock because the assignment might not be atomic. */
    spinlock_acquire(&ptable_lock);
    proc->state = READY;
    spinlock_release(&ptable_lock);
}


/**
 * Fork a new process that is a duplicate of the caller process. Caller
 * is the parent process and the new one is the child process. Scheduling
 * timeslice length of the new process must be an integer in [1, 16].
 * 
 * Returns child pid in parent, 0 in child, and -1 if failed, just like
 * UNIX fork().
 */
int8_t
process_fork(uint8_t timeslice)
{
    if (timeslice < 1 || timeslice > 16)
        return -1;

    process_t *parent = running_proc();

    /** Get a slot in the ptable. */
    process_t *child = _alloc_new_process();
    if (child == NULL) {
        warn("fork: failed to allocate new child process");
        return -1;
    }

    /**
     * Create the new process's page directory, and then copy over the
     * parent process page directory, mapping all mapped-pages for the
     * child process to physical frames along the way.
     */
    child->pgdir = (pde_t *) salloc_page();
    if (child->pgdir == NULL) {
        warn("fork: cannot allocate level-1 directory, out of kheap memory?");
        sfree_page((char *) child->kstack);         // Maybe use goto.
        child->kstack = 0;
        child->pid = 0;
        child->state = UNUSED;
        return -1;
    }
    memset(child->pgdir, 0, sizeof(pde_t) * PDES_PER_PAGE);

    uint32_t vaddr_btm = 0;     /** Kernel-mapped. */
    while (vaddr_btm < PHYS_MAX) {
        pte_t *pte = paging_walk_pgdir(child->pgdir, vaddr_btm, true);
        if (pte == NULL) {
            warn("fork: cannot allocate level-2 table, out of kheap memory?");
            paging_destroy_pgdir(child->pgdir);     // Maybe use goto.
            child->pgdir = NULL;
            sfree_page((char *) child->kstack);
            child->kstack = 0;
            child->pid = 0;
            child->state = UNUSED;
            return -1;
        }
        paging_map_kpage(pte, vaddr_btm);

        vaddr_btm += PAGE_SIZE;
    }

    if (!paging_copy_range(child->pgdir, parent->pgdir,
                           USER_BASE, parent->heap_high)
        || !paging_copy_range(child->pgdir, parent->pgdir,
                              parent->stack_low, USER_MAX)) {
        warn("fork: failed to copy parent memory state over to child");
        paging_unmap_range(child->pgdir, USER_BASE, parent->heap_high);
        paging_unmap_range(child->pgdir, parent->stack_low, USER_MAX);
        paging_destroy_pgdir(child->pgdir);         // Maybe use goto.
        child->pgdir = NULL;
        sfree_page((char *) child->kstack);
        child->kstack = 0;
        child->pid = 0;
        child->state = UNUSED;
        return -1;
    }

    child->stack_low = parent->stack_low;
    child->heap_high = parent->heap_high;

    child->timeslice = timeslice;

    /** Child shares the same set of current open files with parent. */
    for (size_t i = 0; i < MAX_FILES_PER_PROC; ++i) {
        if (parent->files[i] != NULL) {
            child->files[i] = parent->files[i];
            file_ref(parent->files[i]);
        }
    }

    /** Child inherits parent's working directory. */
    child->cwd = parent->cwd;
    inode_ref(parent->cwd);

    /**
     * Copy the trap state of parent to the child. Child should resume
     * execution at the same where place where parent is at right after
     * `fork()` call.
     */
    memcpy(child->trap_state, parent->trap_state, sizeof(interrupt_state_t));
    child->trap_state->eax = 0;     /** `fork()` returns 0 in child. */

    child->parent = parent;
    strncpy(child->name, parent->name, sizeof(parent->name));

    child->killed = false;

    int8_t child_pid = child->pid;

    /** Needed to hold lock because the assignment might not be atomic. */
    spinlock_acquire(&ptable_lock);
    child->state = READY;
    spinlock_release(&ptable_lock);

    return child_pid;               /** Returns child pid in parent. */
}


/**
 * Block the running process on the given reason.
 * 
 * Must be called with `ptable_lock` held, and exactly only `ptable_lock`
 * held (so the CPU's `cli_dpeth` is 1. A common pattern would be like:
 * 
 *   spinlock_acquire(&ptable_lock);
 *   spinlock_release(&something_lock);
 *
 *   proc->wait_something = ...;
 *   process_block(ON_SOMETHING);
 *   proc->wait_something = NULL;
 *
 *   spinlock_release(&ptable_lock);
 *   spinlock_acquire(&something_lock);
 */
inline void
process_block(process_block_on_t reason)
{
    assert(spinlock_locked(&ptable_lock));
    process_t *proc = running_proc();

    proc->block_on = reason;
    proc->state = BLOCKED;

    /** Must yield with `ptable_lock` held. */
    yield_to_scheduler();
}

/**
 * Unblock a process by setting it to READY state and clear the reason.
 * Must be called with `ptable_lock` held.
 */
inline void
process_unblock(process_t *proc)
{
    assert(spinlock_locked(&ptable_lock));
    proc->block_on = NOTHING;
    proc->state = READY;
}


/** Terminate a process. */
void
process_exit(void)
{
    process_t *proc = running_proc();
    assert(proc != initproc);

    /** Close all open files. */
    for (size_t i = 0; i < MAX_FILES_PER_PROC; ++i) {
        if (proc->files[i] != NULL) {
            file_put(proc->files[i]);
            proc->files[i] = NULL;
        }
    }

    inode_put(proc->cwd);
    proc->cwd = NULL;

    spinlock_acquire(&ptable_lock);

    /** Parent might be blocking due to waiting. */
    if (proc->parent->state == BLOCKED && proc->parent->block_on == ON_WAIT)
        process_unblock(proc->parent);

    /**
     * A process must have waited all its children before calling `exit()`
     * itself. Any children still in the process table becomes a:
     *   - "Orphan" process, if it is still running. Pass it to the `init`
     *     process for later "reaping";
     *   - "Zombie" process, if it has terminated. In this case, besides
     *     passing to `init`, we should immediately wake up init as well.
     *
     * The `init` process should be executing an infinite loop of `wait()`.
     */
    for (process_t *child = ptable; child < &ptable[MAX_PROCS]; ++child) {
        if (child->parent == proc) {
            child->parent = initproc;
            if (child->state == TERMINATED)
                process_unblock(initproc);
        }
    }

    /** Go back to scheduler context. */
    proc->state = TERMINATED;
    yield_to_scheduler();
    
    error("exit: process gets re-scheduled after termination");
}


/** Sleep for specified number of timer ticks. */
void
process_sleep(uint32_t sleep_ticks)
{
    process_t *proc = running_proc();

    spinlock_acquire(&timer_tick_lock);
    uint32_t curr_tick = timer_tick;
    spinlock_release(&timer_tick_lock);

    uint32_t target_tick = curr_tick + sleep_ticks;
    proc->target_tick = target_tick;

    spinlock_acquire(&ptable_lock);
    process_block(ON_SLEEP);
    spinlock_release(&ptable_lock);

    /** Could be re-scheduled only if `timer_tick` passed `target_tick`. */
}


/**
 * Wait for any child process's exit. A child process could have already
 * exited and becomes a zombie - in this case, it won't block and will
 * just return the first zombie child it sees in ptable.
 *
 * Cleaning up of children ptable entries is done in `wait()` as well.
 *
 * Returns the pid of the waited child, or -1 if don't have kids.
 */
int8_t
process_wait(void)
{
    process_t *proc = running_proc();
    uint32_t child_pid;

    spinlock_acquire(&ptable_lock);

    while (1) {
        bool have_kids = false;

        for (process_t *child = ptable; child < &ptable[MAX_PROCS]; ++child) {
            if (child->parent != proc)
                continue;

            have_kids = true;

            if (child->state == TERMINATED) {
                /** Found one, clean up its state. */
                child_pid = child->pid;

                sfree_page((char *) child->kstack);
                child->kstack = 0;

                paging_unmap_range(child->pgdir, USER_BASE, child->heap_high);
                paging_unmap_range(child->pgdir, child->stack_low, USER_MAX);
                paging_destroy_pgdir(child->pgdir);

                child->pid = 0;
                child->parent = NULL;
                child->name[0] = '\0';
                child->state = UNUSED;

                spinlock_release(&ptable_lock);
                return child_pid;
            }
        }

        /** Dont' have children. */
        if (!have_kids || proc->killed) {
            spinlock_release(&ptable_lock);
            return -1;
        }

        /**
         * Otherwise, some child process is still running. Block until
         * a child wakes me up at its exit.
         */
        process_block(ON_WAIT);

        /** Could be re-scheduled after being woken up. */
    }
}


/** Force to kill a process by pid. Returns -1 if given pid not found. */
int8_t
process_kill(int8_t pid)
{
    spinlock_acquire(&ptable_lock);

    for (process_t *proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->pid == pid) {
            proc->killed = true;

            /** Wake it up in case it is blocking on anything. */
            if (proc->state == BLOCKED)
                process_unblock(proc);

            spinlock_release(&ptable_lock);
            return 0;
        }
    }

    spinlock_release(&ptable_lock);
    return -1;
}
