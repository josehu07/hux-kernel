/**
 * User program system calls library.
 */


#include "../../src/interrupt/syscall.h"


/**
 * Using a macro to define the syscall library functions, since each of
 * them is just inserting the corresponding syscall number into EAX and
 * doing `INT $INT_NO_SYSCALL` (0x40).
 */
#define SYSLIB_DEFINE(name, no) \
    .global name;            \
    .type name, @function;   \
    name:                    \
        movl $no, %eax;      \
        int $INT_NO_SYSCALL; \
        ret


SYSLIB_DEFINE(hello, SYSCALL_HELLO);
