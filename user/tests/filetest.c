/**
 * User test program - file system operations.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../lib/debug.h"
#include "../lib/printf.h"
#include "../lib/syscall.h"


void
main(int argc, char *argv[])
{
    (void) argc;    // Unused.
    (void) argv;
    
    char dirname[128] = "temp";
    char filepath[128] = "temp/test.txt";
    char filename[128] = "test.txt";

    printf("[P] Created dir '%s' -> %d\n", dirname, create(dirname, CREATE_DIR));
    printf("[P] Created file '%s' -> %d\n", filepath, create(filepath, CREATE_FILE));
    printf("[P] Changed cwd to '%s' -> %d\n", dirname, chdir(dirname));

    int8_t pid = fork(0);
    assert(pid >= 0);

    if (pid == 0) {
        // Child.
        char cwd[100];
        printf("[C] Called getcwd -> %d\n", getcwd(cwd, 100));
        printf("    cwd: %s\n", cwd);
        int8_t fd = open(filename, OPEN_WR);
        printf("[C] Opened file '%s' -> %d\n", filename, fd);
        printf("[C] Written to fd %d -> %d\n", fd, write(fd, "AAAAA", 5));
        printf("    src: %s\n", "AAAAA");
        exit();

    } else {
        // Parent.
        assert(wait() == pid);
        printf("[P] Changed cwd to '%s' -> %d\n", "./..", chdir("./.."));
        int8_t fd = open(filepath, OPEN_RD);
        printf("[P] Opened file '%s' -> %d\n", filepath, fd);
        char buf[6] = {0};
        printf("[P] Read from fd %d -> %d\n", fd, read(fd, buf, 5));
        printf("    dst: %s\n", buf);
        printf("[P] Closing fd %d -> %d\n", fd, close(fd));
        printf("[P] Removing file '%s' -> %d\n", filepath, remove(filepath));
        printf("[P] Removing dir '%s' -> %d\n", dirname, remove(dirname));
    }

    exit();
}
