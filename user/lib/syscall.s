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


SYSCALL_LIBGEN  getpid,   SYSCALL_GETPID
SYSCALL_LIBGEN  fork,     SYSCALL_FORK
SYSCALL_LIBGEN  exit,     SYSCALL_EXIT
SYSCALL_LIBGEN  sleep,    SYSCALL_SLEEP
SYSCALL_LIBGEN  wait,     SYSCALL_WAIT
SYSCALL_LIBGEN  kill,     SYSCALL_KILL
SYSCALL_LIBGEN  tprint,   SYSCALL_TPRINT
SYSCALL_LIBGEN  uptime,   SYSCALL_UPTIME
SYSCALL_LIBGEN  kbdstr,   SYSCALL_KBDSTR
SYSCALL_LIBGEN  setheap,  SYSCALL_SETHEAP
SYSCALL_LIBGEN  open,     SYSCALL_OPEN
SYSCALL_LIBGEN  close,    SYSCALL_CLOSE
SYSCALL_LIBGEN  create,   SYSCALL_CREATE
SYSCALL_LIBGEN  remove,   SYSCALL_REMOVE
SYSCALL_LIBGEN  read,     SYSCALL_READ
SYSCALL_LIBGEN  write,    SYSCALL_WRITE
SYSCALL_LIBGEN  chdir,    SYSCALL_CHDIR
SYSCALL_LIBGEN  getcwd,   SYSCALL_GETCWD
SYSCALL_LIBGEN  exec,     SYSCALL_EXEC
SYSCALL_LIBGEN  fstat,    SYSCALL_FSTAT
SYSCALL_LIBGEN  seek,     SYSCALL_SEEK
SYSCALL_LIBGEN  shutdown, SYSCALL_SHUTDOWN
