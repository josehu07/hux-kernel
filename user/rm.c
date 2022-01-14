/**
 * Command line utility - remove file or directory.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"


static bool
_dir_is_empty(int8_t fd)
{
    /** Empty dir only has '.' and '..'. */
    size_t count = 0;
    dentry_t dentry;
    while (read(fd, (char *) &dentry, sizeof(dentry_t)) == sizeof(dentry_t)) {
        if (dentry.valid == 1)
            count++;
    }

    return count <= 2;
}


static void
_remove_file(char *path, bool allow_dir)
{
    /** If path does not exist, fail. */
    int8_t fd = open(path, OPEN_RD);
    if (fd < 0) {
        warn("rm: cannot open path '%s'", path);
        return;
    }

    file_stat_t stat;
    if (fstat(fd, &stat) != 0) {
        warn("rm: cannot get stat of '%s'", path);
        close(fd);
        return;
    }

    if (stat.type == INODE_TYPE_DIR) {
        /** If path is dir but dir not allowed, fail. */
        if (!allow_dir) {
            warn("rm: path '%s' is directory", path);
            close(fd);
            return;
        }
        /** If path is non-empty dir, fail. */
        if (!_dir_is_empty(fd)) {
            warn("rm: directory '%s' is not empty", path);
            close(fd);
            return;
        }
    }

    close(fd);
    int ret = remove(path);
    if (ret != 0)
        warn("rm: remove '%s' failed", path);
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

    bool allow_dir = false;
    if (strncmp(argv[1], "-r", 2) == 0)
        allow_dir = true;

    char *path;
    if (allow_dir) {
        if (argc != 3)
            _print_help_exit(argv[0]);
        path = argv[2];
    } else {
        if (argc != 2)
            _print_help_exit(argv[0]);
        path = argv[1];
    }

    _remove_file(path, allow_dir);
    exit();
}
