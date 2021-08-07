/**
 * Syscalls related to communication with external devices other than
 * the VGA terminal display.
 */


#include <stdint.h>

#include "sysdev.h"
#include "timer.h"
#include "keyboard.h"

#include "../common/intstate.h"

#include "../interrupt/syscall.h"


/** int32_t uptime(void); */
int32_t
syscall_uptime(void)
{
    cli_push();
    uint32_t curr_tick = timer_tick;
    cli_pop();

    return (int32_t) (curr_tick * 1000 / TIMER_FREQ_HZ);
}

/** int32_t kbdstr(char *buf, int32_t len); */
int32_t
syscall_kbdstr(void)
{
    char *buf;
    int32_t len;

    if (!sysarg_get_int(1, &len))
        return SYS_FAIL_RC;
    if (len <= 0)
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(0, &buf, len))
        return SYS_FAIL_RC;

    return (int32_t) (keyboard_getstr(buf, len));
}
