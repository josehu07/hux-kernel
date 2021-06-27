/**
 * System call-related definitions and handler wrappers.
 */


#include <stdint.h>
#include <stddef.h>

#include "syscall.h"

#include "../common/debug.h"

#include "../interrupt/isr.h"

#include "../process/layout.h"
#include "../process/process.h"
#include "../process/scheduler.h"
#include "../process/sysproc.h"


/** Array of individual handlers: void -> int32_t functions. */
static syscall_t syscall_handlers[] = {
    [SYSCALL_GETPID]    syscall_getpid,
    [SYSCALL_FORK]      syscall_fork,
    [SYSCALL_EXIT]      syscall_exit,
    [SYSCALL_SLEEP]     syscall_sleep,
    [SYSCALL_WAIT]      syscall_wait,
    [SYSCALL_KILL]      syscall_kill
};

#define NUM_SYSCALLS ((int32_t) (sizeof(syscall_handlers) / sizeof(syscall_t)))


/**
 * Centralized syscall handler entry.
 *   - The trap state holds the syscall number in register EAX
 *     and the arguments on user stack;
 *   - Returns a return code back to register EAX.
 *
 * User syscall library should do syscalls following this rule.
 */
void
syscall(interrupt_state_t *state)
{
    int32_t syscall_no = state->eax;

    if (syscall_no <= 0 || syscall_no >= NUM_SYSCALLS) {
        warn("syscall: unknwon syscall number %d", syscall_no);
        state->eax = SYS_FAIL_RC;
    } else if (syscall_handlers[syscall_no] == NULL) {
        warn("syscall: null handler for syscall # %d", syscall_no);
        state->eax = SYS_FAIL_RC;
    } else {
        syscall_t handler = syscall_handlers[syscall_no];
        state->eax = handler();
    }
}


/**
 * Get syscall arguments on the user stack.
 *   - state->esp is the current user ESP;
 *   - 0(state->esp) is the return address, so skip;
 *   - starting from 4(state->esp) are the arguments, from stack
 *     bottom -> top are user lib arguments from left -> right.
 */

/**
 * Fetch the n-th (starting from 0-th) 32bit integer. Returns true on
 * success and false if address not in user stack.
 */
bool
sysarg_get_int(int8_t n, int32_t *ret)
{
    process_t *proc = running_proc();
    uint32_t addr = (proc->trap_state->esp) + 4 + (4 * n);

    /** If not in valid user stack region. */
    if (addr < proc->stack_low || addr + 4 > USER_MAX) {
        warn("sysarg_get_int: invalid arg addr %p for %s", addr, proc->name);
        return false;
    }

    *ret = *((int32_t *) addr);
    return true;
}

/**
 * Fetch the n-th (starting from 0-th) 32-bit argument and interpret as
 * a pointer to a bytes array of length `len`. Returns true on success
 * and false if address invalid.
 */
bool
sysarg_get_mem(int8_t n, char **mem, size_t len)
{
    process_t *proc = running_proc();
    uint32_t ptr;

    if (!sysarg_get_int(n, (int32_t *) &ptr)) {
        warn("sysarg_get_mem: inner sysarg_get_int failed");
        return false;
    }

    if (ptr >= USER_MAX || ptr + len > USER_MAX || ptr < USER_BASE
        || (ptr >= proc->heap_high && ptr < proc->stack_low)
        || (ptr + len > proc->heap_high && ptr + len <= proc->stack_low)
        || (ptr < proc->heap_high && ptr + len > proc->heap_high)) {
        warn("sysarg_get_mem: invalid mem %p w/ len %d for %s",
             ptr, len, proc->name);
        return false;
    }

    *mem = (char *) ptr;
    return true;
}

/**
 * Fetch the n-th (starting from 0-th) 32-bit argument and interpret as
 * a pointer to a string. Returns the length of string actually parsed
 * on success, and -1 if address invalid or the string is not correctly
 * null-terminated.
 */
int32_t
sysarg_get_str(int8_t n, char **str)
{
    process_t *proc = running_proc();
    uint32_t ptr;

    if (!sysarg_get_int(n, (int32_t *) &ptr)) {
        warn("sysarg_get_str: inner sysarg_get_int failed");
        return -1;
    }

    if (ptr >= USER_MAX || ptr < USER_BASE
        || (ptr >= proc->heap_high && ptr < proc->stack_low)) {
        warn("sysarg_get_str: invalid str %p for %s",
             ptr, proc->name);
        return -1;
    }

    char *bound = ptr < proc->heap_high ? (char *) proc->heap_high
                                        : (char *) USER_MAX;
    for (char *c = (char *) ptr; c < bound; ++c) {
        if (*c == '\0') {
            *str = (char *) ptr;
            return c - (char *) ptr;
        }
    }
    return -1;
}
