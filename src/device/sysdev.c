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

#include "../filesys/block.h"
#include "../device/idedisk.h"
#include "../common/string.h"
#include "../common/printf.h"


/** int32_t uptime(void); */
int32_t
syscall_uptime(void)
{
    cli_push();
    uint32_t curr_tick = timer_tick;
    cli_pop();

    block_request_t req_w;
    req_w.valid = true;
    req_w.dirty = true;
    req_w.block_no = 2;
    memset(req_w.data, 'A', BLOCK_SIZE);
    idedisk_do_req(&req_w);
    printf("Written a block of char 'A's @ block index %u\n", req_w.block_no);

    block_request_t req_r;
    req_r.valid = false;
    req_r.dirty = false;
    req_r.block_no = 2;
    idedisk_do_req(&req_r);
    req_r.data[10] = '\0';
    printf("First 10 bytes read: %s\n", req_r.data);

    return (int32_t) (curr_tick * 1000 / TIMER_FREQ_HZ);
}

/** int32_t kbdstr(char *buf, uint32_t len); */
int32_t
syscall_kbdstr(void)
{
    char *buf;
    uint32_t len;

    if (!sysarg_get_uint(1, &len))
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(0, &buf, len))
        return SYS_FAIL_RC;

    return (int32_t) (keyboard_getstr(buf, len));
}
