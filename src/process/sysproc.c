/**
 * Syscalls related to process state & operations.
 */


#include <stdint.h>

#include "sysproc.h"

#include "../common/printf.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** int32_t hello(int32_t num, char *mem, int32_t len, char *str); */
int32_t
syscall_hello(void)
{
    int32_t num, len;
    char *mem, *str;

    if (!sysarg_get_int(0, &num))
        return SYS_FAIL_RC;
    if (!sysarg_get_int(2, &len))
        return SYS_FAIL_RC;
    if (len <= 0)
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(1, &mem, len))
        return SYS_FAIL_RC;
    if (sysarg_get_str(3, &str) < 0)
        return SYS_FAIL_RC;

    process_t *proc = running_proc();
    printf("From sysall_hello handler: Hello, %s!\n", proc->name);
    printf("  num: %d, mem[0]: %c, str: %s\n", num, mem[0], str);

    return 0;
}
