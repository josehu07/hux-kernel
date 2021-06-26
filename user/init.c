/**
 * The init user program to be embedded. Runs in user mode.
 */


#include "lib/syscall.h"


void
main(void)
{
    int32_t num = 7913;

    char mem[8] = "ABCDEFG";
    int32_t len = 7;

    char *str = "This is init!";

    hello(num, mem, len, str);

    asm volatile ( "hlt" );
}
