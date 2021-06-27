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


SYSCALL_LIBGEN  getpid, SYSCALL_GETPID
SYSCALL_LIBGEN  fork,   SYSCALL_FORK
SYSCALL_LIBGEN  exit,   SYSCALL_EXIT
SYSCALL_LIBGEN  sleep,  SYSCALL_SLEEP
SYSCALL_LIBGEN  wait,   SYSCALL_WAIT
SYSCALL_LIBGEN  kill,   SYSCALL_KILL
SYSCALL_LIBGEN  tprint, SYSCALL_TPRINT
