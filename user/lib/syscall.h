/**
 * User program system calls library.
 */


#include <stdint.h>


/** Externed from ASM `syscall.s`. */
extern int32_t hello(int32_t num, char *mem, int32_t len, char *str);
