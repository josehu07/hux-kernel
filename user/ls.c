/**
 * Command line utility - list directory.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/syscall.h"
#include "lib/printf.h"
#include "lib/debug.h"
#include "lib/string.h"


#define CONCAT_BUF_SIZE 300


static char *
_get_filename(char *path)
{
    int32_t idx;
    for (idx = strlen(path) - 1; idx >= 0; --idx) {
        if (path[idx] == '/')
            return &path[idx + 1];
    }
    return path;
}

static void
_print_file_stat(char *filename, file_stat_t *stat)
{
    bool is_dir = stat->type == INODE_TYPE_DIR;
    printf("%5u %s %8u ", stat->inumber, is_dir ? "D" : "F", stat->size);
    cprintf(is_dir ? VGA_COLOR_LIGHT_BLUE : VGA_COLOR_LIGHT_GREY,
            "%s\n", filename);
}


static void
_list_directory(char *path)
{
    int8_t fd = open(path, OPEN_RD);
    if (fd < 0) {
        warn("ls: cannot open path '%s'", path);
        return;
    }

    file_stat_t stat;
    if (fstat(fd, &stat) != 0) {
        warn("ls: cannot get stat of '%s'", path);
        close(fd);
        return;
    }

    /** Listing on a regular file. */
    if (stat.type == INODE_TYPE_FILE) {
        _print_file_stat(_get_filename(path), &stat);
        close(fd);
        return;
    }

    /**
     * Listing a directory, then read out its content, interpret as an
     * array of directory entries.
     */
    char concat_buf[CONCAT_BUF_SIZE];
    strncpy(concat_buf, path, CONCAT_BUF_SIZE - 2);
    if (CONCAT_BUF_SIZE - 1 - strlen(concat_buf) < MAX_FILENAME) {
        warn("ls: path '%s' too long", path);
        close(fd);
        return;
    }
    char *name_buf = &concat_buf[strlen(concat_buf)];
    *(name_buf++) = '/';
    *name_buf = '\0';

    dentry_t dentry;
    while (read(fd, (char *) &dentry, sizeof(dentry_t)) == sizeof(dentry_t)) {
        if (dentry.valid != 1)
            continue;

        strncpy(name_buf, dentry.filename, MAX_FILENAME);
        int8_t inner_fd = open(concat_buf, OPEN_RD);
        if (inner_fd < 0) {
            warn("ls: cannot open path '%s'", concat_buf);
            close(fd);
            return;
        }

        file_stat_t inner_stat;
        if (fstat(inner_fd, &inner_stat) != 0) {
            warn("ls: cannot get stat of '%s'", concat_buf);
            close(inner_fd);
            close(fd);
            return;
        }

        _print_file_stat(dentry.filename, &inner_stat);
        close(inner_fd);
    }

    close(fd);
}


static void
_print_help_exit(char *me)
{
    printf("Usage: %s [-h] [path [paths]]\n", me);
    exit();
}

void
main(int argc, char *argv[])
{
    if (argc < 2) {
        _list_directory(".");
        exit();
    }

    if (strncmp(argv[1], "-h", 2) == 0)
        _print_help_exit(argv[0]);

    if (argc == 2) {
        _list_directory(argv[1]);
        exit();
    }

    /** If multiple directories, display as multiple sections. */
    for (int i = 1; i < argc; ++i) {
        printf("%s:\n", argv[i]);
        _list_directory(argv[i]);
        if (i < argc - 1)
            printf("\n");
    }
    exit();
}
