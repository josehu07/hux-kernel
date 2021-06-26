/**
 * User program system calls library.
 */


.include "syslist.s"


/**
 * Using an auto-generation macro, since every syscall expect the same
 * thing of putting arguments on stack, setting EAX to the number, etc.
 */
.macro SYSCALL_LIBGEN name, no
    .global \name
    .type \name, @function
    \name:
        movl $\no, %eax
        int $INT_NO_SYSCALL
        ret
.endm


SYSCALL_LIBGEN hello, SYSCALL_HELLO
