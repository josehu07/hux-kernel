/**
 * The init user program to be embedded. Runs in user mode.
 */


#include <stdint.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/debug.h"


void
main(void)
{
    // info("init: starting the shell process...");

    int8_t shell_pid = fork(0);
    if (shell_pid < 0) {
        error("init: failed to fork a child process");
        exit();
    }

    if (shell_pid == 0) {
        /** Child: exec the command line shell. */
        char *argv[2];
        argv[0] = "shell";
        argv[1] = NULL;
        exec("shell", argv);
        error("init: failed to exec the shell program");

    } else {
        /** Parent: loop in catching zombie processes. */
        int8_t wait_pid;
        do {
            wait_pid = wait();
            if (wait_pid > 0 && wait_pid != shell_pid)
                warn("init: caught zombie process %d", wait_pid);
        } while (wait_pid > 0 && wait_pid != shell_pid);
        error("init: the shell process exits, should not happen");
    }
}
