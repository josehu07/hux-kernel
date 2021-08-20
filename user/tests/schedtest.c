/**
 * User test program - weighted scheduling.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../lib/debug.h"
#include "../lib/printf.h"
#include "../lib/syscall.h"


static void
_test_child_workload(void)
{
    int32_t res = 0;
    for (int32_t i = 12345; i < 57896; ++i)
        for (int32_t j = i; j > 12345; --j)
            res += (i * j) % 567;
    printf("res %d: %d\n", getpid(), res);
}


void
main(int argc, char *argv[])
{
    (void) argc;    // Unused.
    (void) argv;

    int8_t i;

    printf("parent: forking...\n");
    for (i = 1; i <= 3; ++i) {
        int8_t pid = fork(i*4);
        if (pid < 0)
            error("parent: forking child i=%d failed", i);
        if (pid == 0) {
            // Child.
            _test_child_workload();
            exit();
        } else
            printf("parent: forked child pid=%d, timeslice=%d\n", pid, i*4);
    }

    printf("parent: waiting...\n");
    for (i = 1; i <= 3; ++i) {
        int8_t pid = wait();
        printf("parent: waited child pid=%d\n", pid);
    }

    exit();
}
