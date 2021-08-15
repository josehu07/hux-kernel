/**
 * Lock implementation that blocks the calling process on `acquire()` if
 * the lock is locked. Can only be used under process context.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "parklock.h"
#include "spinlock.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** Returns true if the lock is currently held by the caller process. */
bool
parklock_holding(parklock_t *lock)
{
    spinlock_acquire(&(lock->lock));
    bool held = lock->locked && (lock->holder_pid == running_proc()->pid);
    spinlock_release(&(lock->lock));

    return held;
}


/**
 * Acquire the lock, blocks (parks) the caller if the lock is currently held
 * by some other process.
 */
void
parklock_acquire(parklock_t *lock)
{
    process_t *proc = running_proc();

    spinlock_acquire(&(lock->lock));

    /**
     * Park until lock is released and I'm the first one scheduled among
     * woken up process waiting on this lock.
     */
    while (lock->locked) {
        /** Must hold ptable lock when yielding. */
        spinlock_acquire(&ptable_lock);
        spinlock_release(&lock->lock);

        proc->wait_lock = lock;
        process_block(ON_LOCK);
        proc->wait_lock = NULL;

        spinlock_release(&ptable_lock);
        spinlock_acquire(&(lock->lock));
    }

    lock->locked = true;
    lock->holder_pid = proc->pid;

    spinlock_release(&(lock->lock));
}

/** Release the lock and wake up waiters. */
void
parklock_release(parklock_t *lock)
{
    spinlock_acquire(&(lock->lock));

    lock->locked = false;
    lock->holder_pid = 0;

    /**
     * Wake up all waiter process - the first one getting scheduled among
     * them will be the next one succeeding in acquiring this lock.
     */
    spinlock_acquire(&ptable_lock);
    for (process_t *proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->state == BLOCKED && proc->block_on == ON_LOCK
            && proc->wait_lock == lock) {
            process_unblock(proc);
        }
    }
    spinlock_release(&ptable_lock);

    spinlock_release(&(lock->lock));
}


/** Initialize the parking lock. */
void
parklock_init(parklock_t *lock, const char *name)
{
    spinlock_init(&(lock->lock), "parklock's internal spinlock");
    lock->name = name;
    lock->locked = false;
    lock->holder_pid = 0;
}
