/**
 * Syscalls related to process state & operations.
 */


#include <stdint.h>

#include "sysproc.h"

#include "../common/printf.h"

#include "../device/timer.h"

#include "../interrupt/syscall.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** int8_t getpid(void); */
int32_t
syscall_getpid(void)
{
    return running_proc()->pid;
}

/** int8_t fork(void); */
int32_t
syscall_fork(void)
{
    return process_fork();
}

/** void exit(void); */
int32_t
syscall_exit(void)
{
    process_exit();
    return 0;   /** Not reached. */
}

/** int8_t sleep(int32_t millisecs); */
int32_t
syscall_sleep(void)
{
    int32_t millisecs;
    
    if (!sysarg_get_int(0, &millisecs))
        return SYS_FAIL_RC;
    if (millisecs < 0)
        return SYS_FAIL_RC;

    uint32_t sleep_ticks = millisecs * TIMER_FREQ_HZ / 1000;
    process_sleep(sleep_ticks);

    return 0;
}

/** int8_t wait(void); */
int32_t
syscall_wait(void)
{
    return process_wait();
}

/** int8_t kill(int8_t pid); */
int32_t
syscall_kill(void)
{
    int32_t pid;

    if (!sysarg_get_int(0, &pid))
        return SYS_FAIL_RC;
    if (pid < 0)
        return SYS_FAIL_RC;

    return process_kill(pid);
}
