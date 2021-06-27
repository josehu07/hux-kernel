/**
 * The init user program to be embedded. Runs in user mode.
 */


#include "lib/syscall.h"


static void
_fake_halt(void)
{
    while (1)
        sleep(1000);
}

void
main(void)
{
    int32_t mypid = getpid();
    int32_t pid = fork();

    if (pid < 0)
        _fake_halt();

    if (pid == 0) {
        // Child.
        _fake_halt();
    } else {
        // Parent.
        kill(pid);
        _fake_halt();
    }

    exit();
}
