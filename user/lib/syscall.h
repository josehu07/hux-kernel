/**
 * User program system calls library.
 */


#include <stdint.h>


/** Externed from ASM `syscall.s`. */
extern int8_t getpid();
extern int8_t fork();
extern void exit(void);
extern int8_t sleep(int32_t millisecs);
extern int8_t wait(void);
extern int8_t kill(int8_t pid);
