/**
 * Command line utility - put string into file.
 *
 * Due to limited shell cmdline parsing capability, strings containing
 * whitespaces are not supported yet.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"


#define MAX_STR_LEN 256


static int8_t
_open_writable_file(char *path, bool overwrite)
{
    /** If path is not regular writable file, fail. */
    int8_t fd = open(path, OPEN_WR);
    if (fd < 0) {
        warn("put: cannot open path '%s' for write", path);
        return -1;
    }

    file_stat_t stat;
    if (fstat(fd, &stat) != 0) {
        warn("put: cannot get stat of '%s'", path);
        close(fd);
        return -1;
    }

    if (stat.type != INODE_TYPE_FILE) {
        warn("put: path '%s' is not regular file", path);
        close(fd);
        return -1;
    }

    /** If not overwriting, seek to current file end. */
    if (!overwrite) {
        int ret = seek(fd, stat.size);
        if (ret != 0) {
            warn("put: cannot seek to offset %lu", stat.size);
            close(fd);
            return -1;
        }
    }

    return fd;
}


static void
_file_put_str(char *path, char *str, size_t len, bool overwrite, bool newline)
{
    int8_t fd = _open_writable_file(path, overwrite);
    if (fd < 0)
        return;

    size_t bytes_written = write(fd, str, len);
    if (bytes_written != len) {
        warn("put: bytes written %lu != given length %lu", bytes_written, len);
        close(fd);
        return;
    }

    /** If putting newline after string. */
    if (newline) {
        bytes_written = write(fd, "\n", 1);
        if (bytes_written != 1)
            warn("put: newline written %lu != 1", bytes_written);
    }

    close(fd);
}


static void
_print_help_exit(char *me)
{
    printf("Usage: %s [-h] [-o] [-e] file str\n", me);
    exit();
}

void
main(int argc, char *argv[])
{
    if (argc < 2 || strncmp(argv[1], "-h", 2) == 0)
        _print_help_exit(argv[0]);

    int argi = 1;
    bool newline = true, overwrite = false;
    while (true) {
        if (strncmp(argv[argi], "-e", 2) == 0) {
            argi++;
            newline = false;
        } else if (strncmp(argv[argi], "-o", 2) == 0) {
            argi++;
            overwrite = true;
        } else
            break;
    }

    char *path, *str;
    if (argc - argi != 2)
        _print_help_exit(argv[0]);
    path = argv[argi++];
    str  = argv[argi++];

    size_t len = strnlen(str, MAX_STR_LEN);
    if (len == MAX_STR_LEN)
        warn("put: str exceeds max length %lu", MAX_STR_LEN);

    _file_put_str(path, str, len, overwrite, newline);
    exit();
}
