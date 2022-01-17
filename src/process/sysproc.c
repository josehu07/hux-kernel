/**
 * Syscalls related to process state & operations.
 */


#include <stdint.h>

#include "sysproc.h"

#include "../common/printf.h"
#include "../common/debug.h"
#include "../common/port.h"

#include "../device/timer.h"

#include "../interrupt/syscall.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** int32_t getpid(void); */
int32_t
syscall_getpid(void)
{
    return running_proc()->pid;
}

/** int32_t fork(uint32_t timeslice); */
int32_t
syscall_fork(void)
{
    uint32_t timeslice;

    if (!sysarg_get_uint(0, &timeslice))
        return SYS_FAIL_RC;
    if (timeslice > 16) {
        warn("fork: timeslice value cannot be larger than 16");
        return SYS_FAIL_RC;
    }

    if (timeslice == 0)
        return process_fork(running_proc()->timeslice);
    return process_fork(timeslice);
}

/** void exit(void); */
int32_t
syscall_exit(void)
{
    process_exit();
    return 0;   /** Not reached. */
}

/** int32_t sleep(uint32_t millisecs); */
int32_t
syscall_sleep(void)
{
    uint32_t millisecs;
    
    if (!sysarg_get_uint(0, &millisecs))
        return SYS_FAIL_RC;

    uint32_t sleep_ticks = millisecs * TIMER_FREQ_HZ / 1000;
    process_sleep(sleep_ticks);

    return 0;
}

/** int32_t wait(void); */
int32_t
syscall_wait(void)
{
    return process_wait();
}

/** int32_t kill(int32_t pid); */
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

/** void shutdown(void); */
int32_t
syscall_shutdown(void)
{
    /**
     * QEMU-specific!
     * Magic shutdown value of QEMU's default ACPI method.
     */
    outw(0x604, 0x2000);
    return 0;   /** Not reached. */
}
