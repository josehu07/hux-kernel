/**
 * The init user program to be embedded. Runs in user mode.
 */


#include <stdint.h>

#include "lib/syscall.h"
#include "lib/printf.h"


static void
_fake_halt(void)
{
    while (1)
        sleep(1000);
}

void
main(void)
{
    printf("\n");

    int32_t mypid = getpid();
    printf(" Parent: parent gets pid - %d\n", mypid);
    sleep(2000);

    cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 1 --\n");
    printf("  Parent: forking child 1\n");
    int32_t pid1 = fork();
    if (pid1 < 0) {
        cprintf(VGA_COLOR_RED, "  Parent: fork failed\n");
        _fake_halt();
    }
    if (pid1 == 0) {
        // Child.
        printf("  Child1: entering an infinite loop\n");
        _fake_halt();
    } else {
        // Parent.
        printf("  Parent: child 1 has pid - %d\n", pid1);
        sleep(1500);
        printf("  Parent: slept 1.5 secs, going to kill child 1\n");
        kill(pid1);
        wait();
        printf("  Parent: killed child 1\n");
    }

    cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 2 --\n");
    printf("  Current uptime: %d ms\n", uptime());
    printf("  Going to sleep for 2000 ms...\n");
    sleep(2000);
    printf("  Current uptime: %d ms\n", uptime());

    cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 3 --\n");
    printf("  Parent: forking child 2\n");
    int32_t pid2 = fork();
    if (pid2 < 0) {
        cprintf(VGA_COLOR_RED, "  Parent: fork failed\n");
        _fake_halt();
    }
    if (pid2 == 0) {
        // Child.
        printf("  Child2: going to sleep 2 secs\n");
        sleep(2000);
        exit();
    } else {
        // Parent.
        printf("  Parent: child 2 has pid - %d\n", pid2);
        wait();
        printf("  Parent: waited child 2\n");
    }

    cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 4 --\n");
    char kbd_buf[100];
    printf("  Parent: please give an input str: ");
    kbdstr(kbd_buf, 100);
    printf("  Parent: input str from keyboard : %s\n", kbd_buf);

    cprintf(VGA_COLOR_GREEN, "\n Cases done!\n");

    _fake_halt();
}
