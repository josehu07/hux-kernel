/**
 * Spinlock implementation (synonym to `cli_push()`/`cli_pop()` pairs
 * in single-CPU Hux).
 */


#include <stdint.h>
#include <stdbool.h>

#include "spinlock.h"
#include "intstate.h"
#include "debug.h"


/** Returns true if the lock is currently locked. */
bool
spinlock_locked(spinlock_t *lock)
{
    cli_push();
    bool locked = (lock->locked == 1);
    cli_pop();
    
    return locked;
}


/** x86 atomic XCHG instruction wrapper. */
static inline uint32_t
_xchgl(volatile uint32_t *ptr, uint32_t new_val)
{
    uint32_t old_val;
    asm volatile ( "lock; xchgl %0, %1"
                   : "+m" (*ptr), "=a" (old_val)
                   : "1" (new_val) );
    return old_val;
}

/**
 * Loops until the lock is acquired.
 * 
 * Should succeed immediately in Hux since we only have one CPU and any
 * process must not yield when holding a spinlock (which may cause
 * another process that gets scheduled to deadlock on spinning on the
 * lock). Hence, it basically serves as `cli_push()` for now.
 */
void
spinlock_acquire(spinlock_t *lock)
{
    cli_push();

    if (spinlock_locked(lock))
        error("spinlock_acquire: lock %s is already locked", lock->name);

    /** Spins until XCHG gets old value of "unlocked". */
    while (_xchgl(&(lock->locked), 1) != 0) {}

    /** Memory barrier, no loads/stores could cross this point. */
    __sync_synchronize();
}

/** Release the lock. */
void
spinlock_release(spinlock_t *lock)
{
    if (!spinlock_locked(lock))
        error("spinlock_release: lock %s is not locked", lock->name);

    /** Memory barrier, no loads/stores could cross this point. */
    __sync_synchronize();

    /** Atomically assign to 0 (C statement could be non-atomic). */
    asm volatile ( "movl $0, %0" : "+m" (lock->locked) : );

    cli_pop();
}


/** Initialize the spinlock. */
void
spinlock_init(spinlock_t *lock, const char *name)
{
    lock->name = name;
    lock->locked = 0;
}
