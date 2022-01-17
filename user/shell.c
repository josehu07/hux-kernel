/**
 * Basic command line shell.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"
#include "lib/types.h"


#define CWD_BUF_SIZE  256
#define LINE_BUF_SIZE 256
#define MAX_EXEC_ARGS 32

#define ENV_PATH "/"


/** A fancy welcome logo in ASCII. */
static void
_shell_welcome_logo(void)
{
    cprintf(VGA_COLOR_LIGHT_BLUE,
            "\n"
            "                /--/   /--/                                    \n"
            "               /  /   /  /                                     \n"
            "Welcome to    /  /---/  /     /--/   /--/     /--/   /--/   OS!\n"
            "             /  /---/  /     /  /   /  /       | |-/ /         \n"
            "            /  /   /  /     /  /---/  /       / /-| |          \n"
            "           /--/   /--/     /---------/     /--/   |--|         \n"
            "\n");
}

/** Compose & print the prompt. */
static void
_print_prompt(void)
{
    char username[5] = "hush";
    cprintf(VGA_COLOR_GREEN, "%s", username);

    cprintf(VGA_COLOR_DARK_GREY, ":");

    char cwd[CWD_BUF_SIZE];
    memset(cwd, 0, CWD_BUF_SIZE);
    if (getcwd(cwd, CWD_BUF_SIZE - MAX_FILENAME - 1) != 0)
        error("shell: failed to get cwd");
    size_t ret_len = strlen(cwd);
    if (cwd[ret_len - 1] != '/') {
        cwd[ret_len] = '/';
        cwd[ret_len + 1] = '\0';
    }
    cprintf(VGA_COLOR_CYAN, "%s", cwd);

    cprintf(VGA_COLOR_DARK_GREY, "$ ");
}


/** Built-in cmd `cd`: change directory. */
static void
_change_cwd(char *path)
{
    if (path == NULL)   /** empty path, go to "/". */
        path = "/";

    if (chdir(path) != 0)
        warn("shell: cd to path '%s' failed", path);
}

/** Built-in cmd `shutdown`: poweroff machine. */
static void
_shutdown(void)
{
    char answer[LINE_BUF_SIZE];
    printf("Shutting down, confirm? (y/n) ");
    if (kbdstr(answer, LINE_BUF_SIZE) < 0)
        error("shell: failed to get keyboard string");

    if (strncmp(answer, "y", 1) != 0)
        printf("Aborted.\n");
    else {
        printf("Confirmed.\n");
        shutdown();
        /** Not reached. */
    }
}

/** Fork + exec external command executable. */
static void
_fork_exec(char *path, char **argv)
{
    int8_t pid = fork(0);
    if (pid < 0) {
        warn("shell: failed to fork child process");
        return;
    }

    if (pid == 0) {     /** Child. */
        /**
         * Try the executable under current working directory if found.
         * Otherwise, fall through to try the one under ENV_PATH (which
         * is the root directory "/").
         */
        if (open(path, OPEN_RD) < 0) {      /** Not found. */
            char full[CWD_BUF_SIZE];
            snprintf(full, CWD_BUF_SIZE, "%s/%s", ENV_PATH, path);
            exec(full, argv);
        } else 
            exec(path, argv);

        warn("shell: failed to exec '%s'", path);
        exit();
    
    } else {            /** Parent shell. */
        int8_t pid_w = wait();
        if (pid_w != pid)
            warn("shell: waited pid %d does not equal %d", pid_w, pid);
    }
}


/**
 * Parse a command line. Parse the tokens according to whitespaces,
 * interpret the first one as the executable filename, and together
 * with the rest as the argument list.
 */
static int
_parse_tokens(char *line, char **argv)
{
    size_t argi;
    size_t pos = 0;

    for (argi = 0; argi < MAX_EXEC_ARGS - 1; ++argi) {
        while (isspace(line[pos]))
            pos++;
        if (line[pos] == '\0') {
            argv[argi] = NULL;
            return argi;
        }

        argv[argi] = &line[pos];

        while(!isspace(line[pos]))
            pos++;
        if (line[pos] != '\0')
            line[pos++] = '\0';
    }

    argv[argi] = NULL;
    return argi;
}

/**
 * Handle a command line.
 *   - If is a built-in command, handle directly;
 *   - Else, fork a child process to run the command executable.
 */
static void
_handle_cmdline(char *line)
{
    char *argv[MAX_EXEC_ARGS];
    int argc = _parse_tokens(line, argv);
    if (argc <= 0)
        return;
    else if (argc >= MAX_EXEC_ARGS)
        warn("shell: line exceeds max num of args %d", MAX_EXEC_ARGS);

    if (strncmp(argv[0], "cd", 2) == 0)
        _change_cwd(argv[1]);   /** Could be NULL. */
    else if (strncmp(argv[0], "shutdown", 8) == 0)
        _shutdown();
    else
        _fork_exec(argv[0], argv);
}


void
main(int argc, char *argv[])
{
    (void) argc;    // Unused.
    (void) argv;

    _shell_welcome_logo();

    char cmd_buf[LINE_BUF_SIZE];
    memset(cmd_buf, 0, LINE_BUF_SIZE);

    while (1) {
        _print_prompt();
        
        if (kbdstr(cmd_buf, LINE_BUF_SIZE) < 0)
            error("shell: failed to get keyboard string");
        else
            _handle_cmdline(cmd_buf);

        memset(cmd_buf, 0, LINE_BUF_SIZE);
    }

    exit();
}
