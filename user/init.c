/**
 * The init user program to be embedded. Runs in user mode.
 */


#include <stdint.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"
#include "lib/malloc.h"


static void
_fake_halt(void)
{
    while (1)
        sleep(1000);
}


/** An even fancier welcome logo in ASCII. */
static void
_shell_welcome_logo(void)
{
    cprintf(VGA_COLOR_LIGHT_BLUE,
            "\n\n"
            "                /--/   /--/                                  \n"
            "               /  /   /  /                                   \n"
            "Welcome to    /  /---/  /     /--/   /--/     /--/   /--/   !\n"
            "             /  /---/  /     /  /   /  /       | |-/ /       \n"
            "            /  /   /  /     /  /---/  /       / /-| |        \n"
            "           /--/   /--/     /---------/     /--/   |--|       \n"
            "\n");
}

static void
_shell_temp_main(void)
{
    _shell_welcome_logo();

    char cmd_buf[128];
    memset(cmd_buf, 0, 128);

    while (1) {
        printf("temp shell $ ");
        
        if (kbdstr(cmd_buf, 128) < 0)
            warn("shell: failed to get keyboard string");
        else
            printf("%s", cmd_buf);

        memset(cmd_buf, 0, 128);
    }

    // printf("On-stack buffer of size 8200...\n");
    // char buf[8200];
    // buf[0] = 'A';
    // buf[1] = '\0';
    // printf("Variable buf @ %p: %s\n", buf, buf);

    // printf("\nOn-heap allocations & frees...\n");
    // char *buf1 = (char *) malloc(200);
    // printf("Buf1: %p\n", buf1);
    // char *buf2 = (char *) malloc(4777);
    // printf("Buf2: %p\n", buf2);
    // mfree(buf1);
    // char *buf3 = (char *) malloc(8);
    // printf("Buf3: %p\n", buf3);
    // mfree(buf3);
    // mfree(buf2);

    // char kbd_buf[100];
    // printf("Please enter an input str: ");
    // kbdstr(kbd_buf, 100);
    // printf("Fetched str from keyboard: %s\n", kbd_buf);

    // int32_t mypid = getpid();
    // printf(" Parent: parent gets pid - %d\n", mypid);
    // sleep(2000);

    // cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 1 --\n");
    // printf("  Parent: forking child 1\n");
    // int32_t pid1 = fork();
    // if (pid1 < 0) {
    //     cprintf(VGA_COLOR_RED, "  Parent: fork failed\n");
    //     _fake_halt();
    // }
    // if (pid1 == 0) {
    //     // Child.
    //     printf("  Child1: entering an infinite loop\n");
    //     _fake_halt();
    // } else {
    //     // Parent.
    //     printf("  Parent: child 1 has pid - %d\n", pid1);
    //     sleep(1500);
    //     printf("  Parent: slept 1.5 secs, going to kill child 1\n");
    //     kill(pid1);
    //     wait();
    //     printf("  Parent: killed child 1\n");
    // }

    // cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 2 --\n");
    // printf("  Current uptime: %d ms\n", uptime());
    // printf("  Going to sleep for 2000 ms...\n");
    // sleep(2000);
    // printf("  Current uptime: %d ms\n", uptime());

    // cprintf(VGA_COLOR_LIGHT_GREEN, "\n Round 3 --\n");
    // printf("  Parent: forking child 2\n");
    // int32_t pid2 = fork();
    // if (pid2 < 0) {
    //     cprintf(VGA_COLOR_RED, "  Parent: fork failed\n");
    //     _fake_halt();
    // }
    // if (pid2 == 0) {
    //     // Child.
    //     printf("  Child2: going to sleep 2 secs\n");
    //     sleep(2000);
    //     exit();
    // } else {
    //     // Parent.
    //     printf("  Parent: child 2 has pid - %d\n", pid2);
    //     wait();
    //     printf("  Parent: waited child 2\n");
    // }

    // cprintf(VGA_COLOR_GREEN, "\n Cases done!\n");
}


/** A fancy counting-down animation to make booting look better. */
static inline void
_count_down_message(void)
{
    printf("\nBooting finished! Kicking off in -->>   [   ]\b\b\b\b");
    sleep(200);

    cprintf(VGA_COLOR_RED,         "*");
    printf("  ]  ( )\b\b");
    cprintf(VGA_COLOR_WHITE, "3\b\b\b\b\b\b\b");
    sleep(1000);

    cprintf(VGA_COLOR_LIGHT_BROWN, "*");
    printf(" ]  ( )\b\b");
    cprintf(VGA_COLOR_WHITE, "2\b\b\b\b\b\b");
    sleep(1000);

    cprintf(VGA_COLOR_GREEN,       "*");
    printf("]  ( )\b\b");
    cprintf(VGA_COLOR_WHITE, "1\b");
    sleep(1000);

    cprintf(VGA_COLOR_WHITE, "0\n");
}

void
main(void)
{
    _count_down_message();

    while (1) {
        info("init: starting the shell process...");

        int8_t shell_pid = fork(0);
        if (shell_pid < 0) {
            panic("init: failed to fork a child process");
            exit();
        }

        if (shell_pid == 0) {
            // Child.
            _shell_temp_main();
            _fake_halt();
        } else {
            // Parent.
            int8_t wait_pid;
            do {
                wait_pid = wait();
                if (wait_pid > 0 && wait_pid != shell_pid)
                    warn("init: caught zombie process %d", wait_pid);
            } while (wait_pid > 0 && wait_pid != shell_pid);
        }
    }

    exit();
}
