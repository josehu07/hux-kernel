/**
 * Command line utility - dump file content.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"


#define DUMP_BUF_LEN 128
static char dump_buf[DUMP_BUF_LEN];


static void
_dump_file(char *path)
{
    /** If path is not regular readable file, fail. */
    int8_t fd = open(path, OPEN_RD);
    if (fd < 0) {
        warn("put: cannot open path '%s' for read", path);
        return;
    }

    /** Loop to dump all file content. */
    size_t bytes_read;
    while ((bytes_read = read(fd, dump_buf, DUMP_BUF_LEN - 1)) > 0) {
        assert(bytes_read <= DUMP_BUF_LEN - 1);
        dump_buf[bytes_read] = '\0';
        printf("%s", dump_buf);
    }

    close(fd);
}


static void
_print_help_exit(char *me)
{
    printf("Usage: %s [-h] file\n", me);
    exit();
}

void
main(int argc, char *argv[])
{
    if (argc < 2 || strncmp(argv[1], "-h", 2) == 0)
        _print_help_exit(argv[0]);

    if (argc != 2)
        _print_help_exit(argv[0]);
    char *path = argv[1];

    _dump_file(path);
    exit();
}
