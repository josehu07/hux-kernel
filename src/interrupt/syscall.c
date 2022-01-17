/**
 * System call-related definitions and handler wrappers.
 */


#include <stdint.h>
#include <stddef.h>

#include "syscall.h"

#include "../common/debug.h"

#include "../display/sysdisp.h"

#include "../device/sysdev.h"

#include "../interrupt/isr.h"

#include "../memory/sysmem.h"

#include "../process/layout.h"
#include "../process/process.h"
#include "../process/scheduler.h"
#include "../process/sysproc.h"

#include "../filesys/sysfile.h"


/** Array of individual handlers: void -> int32_t functions. */
static syscall_t syscall_handlers[] = {
    [SYSCALL_GETPID]    syscall_getpid,
    [SYSCALL_FORK]      syscall_fork,
    [SYSCALL_EXIT]      syscall_exit,
    [SYSCALL_SLEEP]     syscall_sleep,
    [SYSCALL_WAIT]      syscall_wait,
    [SYSCALL_KILL]      syscall_kill,
    [SYSCALL_TPRINT]    syscall_tprint,
    [SYSCALL_UPTIME]    syscall_uptime,
    [SYSCALL_KBDSTR]    syscall_kbdstr,
    [SYSCALL_SETHEAP]   syscall_setheap,
    [SYSCALL_OPEN]      syscall_open,
    [SYSCALL_CLOSE]     syscall_close,
    [SYSCALL_CREATE]    syscall_create,
    [SYSCALL_REMOVE]    syscall_remove,
    [SYSCALL_READ]      syscall_read,
    [SYSCALL_WRITE]     syscall_write,
    [SYSCALL_CHDIR]     syscall_chdir,
    [SYSCALL_GETCWD]    syscall_getcwd,
    [SYSCALL_EXEC]      syscall_exec,
    [SYSCALL_FSTAT]     syscall_fstat,
    [SYSCALL_SEEK]      syscall_seek,
    [SYSCALL_SHUTDOWN]  syscall_shutdown
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


/** Helpers for getting something from user memory address. */
bool
sysarg_addr_int(uint32_t addr, int32_t *ret)
{
    process_t *proc = running_proc();

    if (addr < proc->stack_low || addr + 4 > USER_MAX) {
        warn("sysarg_addr_int: invalid arg addr %p for %s", addr, proc->name);
        return false;
    }

    *ret = *((int32_t *) addr);
    return true;
}

bool
sysarg_addr_uint(uint32_t addr, uint32_t *ret)
{
    process_t *proc = running_proc();

    if (addr < proc->stack_low || addr + 4 > USER_MAX) {
        warn("sysarg_addr_uint: invalid arg addr %p for %s", addr, proc->name);
        return false;
    }

    *ret = *((uint32_t *) addr);
    return true;
}

bool
sysarg_addr_mem(uint32_t addr, char **mem, size_t len)
{
    process_t *proc = running_proc();

    if (addr >= USER_MAX || addr + len > USER_MAX || addr < USER_BASE
        || (addr >= proc->heap_high && addr < proc->stack_low)
        || (addr + len > proc->heap_high && addr + len <= proc->stack_low)
        || (addr < proc->heap_high && addr + len > proc->heap_high)) {
        warn("sysarg_addr_mem: invalid mem %p w/ len %d for %s",
             addr, len, proc->name);
        return false;
    }

    *mem = (char *) addr;
    return true;
}

int32_t
sysarg_addr_str(uint32_t addr, char **str)
{
    process_t *proc = running_proc();

    if (addr >= USER_MAX || addr < USER_BASE
        || (addr >= proc->heap_high && addr < proc->stack_low)) {
        warn("sysarg_get_str: invalid str %p for %s",
             addr, proc->name);
        return -1;
    }

    char *bound = addr < proc->heap_high ? (char *) proc->heap_high
                                         : (char *) USER_MAX;
    for (char *c = (char *) addr; c < bound; ++c) {
        if (*c == '\0') {
            *str = (char *) addr;
            return c - (char *) addr;
        }
    }
    return -1;
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
    return sysarg_addr_int(addr, ret);
}

/** Same but for uint32_t. */
bool
sysarg_get_uint(int8_t n, uint32_t *ret)
{
    process_t *proc = running_proc();
    uint32_t addr = (proc->trap_state->esp) + 4 + (4 * n);
    return sysarg_addr_uint(addr, ret);
}

/**
 * Fetch the n-th (starting from 0-th) 32-bit argument and interpret as
 * a pointer to a bytes array of length `len`. Returns true on success
 * and false if address invalid.
 */
bool
sysarg_get_mem(int8_t n, char **mem, size_t len)
{
    uint32_t ptr;
    if (!sysarg_get_int(n, (int32_t *) &ptr)) {
        warn("sysarg_get_mem: inner sysarg_get_int failed");
        return false;
    }
    return sysarg_addr_mem(ptr, mem, len);
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
    uint32_t ptr;
    if (!sysarg_get_int(n, (int32_t *) &ptr)) {
        warn("sysarg_get_str: inner sysarg_get_int failed");
        return -1;
    }
    return sysarg_addr_str(ptr, str);
}
