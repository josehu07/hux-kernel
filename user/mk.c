/**
 * Command line utility - create file or directory.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"


static void
_create_file(char *path, bool is_dir)
{
    /** If path exists, fail. */
    int8_t fd = open(path, OPEN_RD);
    if (fd >= 0) {
        warn("mk: path '%s' exists", path);
        close(fd);
        return;
    }

    int ret = create(path, is_dir ? CREATE_DIR : CREATE_FILE);
    if (ret != 0)
        warn("mk: create '%s' failed", path);
}


static void
_print_help_exit(char *me)
{
    printf("Usage: %s [-h] [-r] path\n", me);
    exit();
}

void
main(int argc, char *argv[])
{
    if (argc < 2 || strncmp(argv[1], "-h", 2) == 0)
        _print_help_exit(argv[0]);

    bool is_dir = false;
    if (strncmp(argv[1], "-r", 2) == 0)
        is_dir = true;

    char *path;
    if (is_dir) {
        if (argc != 3)
            _print_help_exit(argv[0]);
        path = argv[2];
    } else {
        if (argc != 2)
            _print_help_exit(argv[0]);
        path = argv[1];
    }

    _create_file(path, is_dir);
    exit();
}
