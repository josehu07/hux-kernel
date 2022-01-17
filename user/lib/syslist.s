/**
 * List of Hux supported sysclls. Should be the same with the list at
 * `src/interrupt/syscall.h`.
 *
 * Using GNU AS constant format.
 */


/** Syscall trap gate number. */
INT_NO_SYSCALL = 64


/** List of known syscalls. */
SYSCALL_GETPID   = 1
SYSCALL_FORK     = 2
SYSCALL_EXIT     = 3
SYSCALL_SLEEP    = 4
SYSCALL_WAIT     = 5
SYSCALL_KILL     = 6
SYSCALL_TPRINT   = 7
SYSCALL_UPTIME   = 8
SYSCALL_KBDSTR   = 9
SYSCALL_SETHEAP  = 10
SYSCALL_OPEN     = 11
SYSCALL_CLOSE    = 12
SYSCALL_CREATE   = 13
SYSCALL_REMOVE   = 14
SYSCALL_READ     = 15
SYSCALL_WRITE    = 16
SYSCALL_CHDIR    = 17
SYSCALL_GETCWD   = 18
SYSCALL_EXEC     = 19
SYSCALL_FSTAT    = 20
SYSCALL_SEEK     = 21
SYSCALL_SHUTDOWN = 22
