/**
 * Lock implementation that blocks the calling process on `acquire()` if
 * the lock is locked. Can only be used under process context.
 */


#ifndef PARKLOCK_H
#define PARKLOCK_H


#include <stdint.h>
#include <stdbool.h>

#include "spinlock.h"


/** Parking lock structure. */
struct parklock {
    bool locked;        /** True if locked, changes must be protected. */
    spinlock_t lock;    /** Internal spinlocks that protects `locked`. */
    int8_t holder_pid;  /** Holder process's PID. */
    const char *name;   /** Lock name for debugging. */
};
typedef struct parklock parklock_t;


void parklock_acquire(parklock_t *lock);
void parklock_release(parklock_t *lock);

bool parklock_holding(parklock_t *lock);

void parklock_init(parklock_t *lock, const char *name);


#endif
