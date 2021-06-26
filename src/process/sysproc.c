/**
 * Syscalls related to process state & operations.
 */


#include <stdint.h>

#include "sysproc.h"

#include "../common/printf.h"

#include "../process/process.h"
#include "../process/scheduler.h"


int32_t
syscall_hello(void)
{
    process_t *proc = running_proc();
    printf("From sysall_hello handler: Hello, %s!\n", proc->name);

    return 0;
}
