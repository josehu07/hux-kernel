/**
 * Syscalls related to terminal printing.
 */


#include <stdint.h>

#include "sysdisp.h"
#include "vga.h"

#include "../common/printf.h"

#include "../interrupt/syscall.h"


/** void tprint(uint32_t color, char *str); */
int32_t
syscall_tprint(void)
{
    uint32_t color;
    char *str;

    if (!sysarg_get_uint(0, &color))
        return SYS_FAIL_RC;
    if (color > 15)
        return SYS_FAIL_RC;
    if (sysarg_get_str(1, &str) < 0)
        return SYS_FAIL_RC;

    cprintf((vga_color_t) color, "%s", str);
    return 0;
}
