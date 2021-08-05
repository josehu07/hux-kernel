/**
 * List of Hux supported sysclls. Should be the same with the list at
 * `src/interrupt/syscall.h`.
 *
 * Using GNU AS constant format.
 */


/** Syscall trap gate number. */
INT_NO_SYSCALL = 64


/** List of known syscalls. */
SYSCALL_GETPID  = 1
SYSCALL_FORK    = 2
SYSCALL_EXIT    = 3
SYSCALL_SLEEP   = 4
SYSCALL_WAIT    = 5
SYSCALL_KILL    = 6
SYSCALL_TPRINT  = 7
SYSCALL_UPTIME  = 8
SYSCALL_KBDSTR  = 9
SYSCALL_SETHEAP = 10
