/**
 * User test program - file system operations.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../lib/debug.h"
#include "../lib/printf.h"
#include "../lib/syscall.h"
#include "../lib/malloc.h"


void
main(int argc, char *argv[])
{
    (void) argc;    // Unused.
    (void) argv;
    
    printf("On-stack buffer of size 8200...\n");
    char buf[8200];
    buf[0] = 'A';
    buf[1] = '\0';
    printf("Variable buf @ %p: %s\n", buf, buf);

    printf("\nOn-heap allocations & frees...\n");
    char *buf1 = (char *) malloc(200);
    printf("Buf1: %p\n", buf1);
    char *buf2 = (char *) malloc(4777);
    printf("Buf2: %p\n", buf2);
    mfree(buf1);
    char *buf3 = (char *) malloc(8);
    printf("Buf3: %p\n", buf3);
    mfree(buf3);
    mfree(buf2);

    exit();
}
