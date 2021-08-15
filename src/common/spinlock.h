/**
 * Spinlock implementation (synonym to `cli_push()`/`cli_pop()` pairs
 * in single-CPU Hux).
 */


#ifndef SPINLOCK_H
#define SPINLOCK_H


#include <stdint.h>
#include <stdbool.h>


/** Simple spinlock structure. */
struct spinlock {
    uint32_t locked;    /** 0 unlocked, 1 locked, changes must be atomic. */
    const char *name;   /** Lock name for debugging. */
};
typedef struct spinlock spinlock_t;


void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

bool spinlock_locked(spinlock_t *lock);

void spinlock_init(spinlock_t *lock, const char *name);


#endif
