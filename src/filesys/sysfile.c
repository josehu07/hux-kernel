/**
 * Syscalls related to process state & operations.
 */


#include <stdint.h>
#include <stdbool.h>

#include "sysfile.h"
#include "vsfs.h"
#include "file.h"
#include "exec.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../interrupt/syscall.h"


/** int32_t open(char *path, uint32_t mode); */
int32_t
syscall_open(void)
{
    char *path;
    uint32_t mode;

    if (sysarg_get_str(0, &path) <= 0)
        return SYS_FAIL_RC;
    if (!sysarg_get_uint(1, &mode))
        return SYS_FAIL_RC;
    if ((mode & (OPEN_RD | OPEN_WR)) == 0) {
        warn("open: mode is neither readable nor writable");
        return SYS_FAIL_RC;
    }

    return filesys_open(path, mode);
}

/** int32_t close(int32_t fd); */
int32_t
syscall_close(void)
{
    int32_t fd;

    if (!sysarg_get_int(0, &fd))
        return SYS_FAIL_RC;
    if (fd < 0 || fd >= MAX_FILES_PER_PROC)
        return SYS_FAIL_RC;

    if (!filesys_close(fd))
        return SYS_FAIL_RC;
    return 0;
}

/** int32_t create(char *path, uint32_t mode); */
int32_t
syscall_create(void)
{
    char *path;
    uint32_t mode;

    if (sysarg_get_str(0, &path) <= 0)
        return SYS_FAIL_RC;
    if (!sysarg_get_uint(1, &mode))
        return SYS_FAIL_RC;
    if ((mode & (CREATE_FILE | CREATE_DIR)) == 0) {
        warn("create: mode is neigher file nor directory");
        return SYS_FAIL_RC;
    }
    if ((mode & CREATE_FILE) != 0 && (mode & CREATE_DIR) != 0) {
        warn("create: mode is both file and directory");
        return SYS_FAIL_RC;
    }

    if (!filesys_create(path, mode))
        return SYS_FAIL_RC;
    return 0;
}

/** int32_t remove(char *path); */
int32_t
syscall_remove(void)
{
    char *path;

    if (sysarg_get_str(0, &path) <= 0)
        return SYS_FAIL_RC;

    if (!filesys_remove(path))
        return SYS_FAIL_RC;
    return 0;
}

/** int32_t read(int32_t fd, char *dst, uint32_t len); */
int32_t
syscall_read(void)
{
    int32_t fd;
    char *dst;
    uint32_t len;

    if (!sysarg_get_int(0, &fd))
        return SYS_FAIL_RC;
    if (fd < 0 || fd >= MAX_FILES_PER_PROC)
        return SYS_FAIL_RC;
    if (!sysarg_get_uint(2, &len))
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(1, &dst, len))
        return SYS_FAIL_RC;

    return filesys_read(fd, dst, len);
}

/** int32_t write(int32_t fd, char *src, uint32_t len); */
int32_t
syscall_write(void)
{
    int32_t fd;
    char *src;
    uint32_t len;

    if (!sysarg_get_int(0, &fd))
        return SYS_FAIL_RC;
    if (fd < 0 || fd >= MAX_FILES_PER_PROC)
        return SYS_FAIL_RC;
    if (!sysarg_get_uint(2, &len))
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(1, &src, len))
        return SYS_FAIL_RC;

    return filesys_write(fd, src, len);
}

/** int32_t chdir(char *path); */
int32_t
syscall_chdir(void)
{
    char *path;

    if (sysarg_get_str(0, &path) <= 0)
        return SYS_FAIL_RC;

    if (!filesys_chdir(path))
        return SYS_FAIL_RC;
    return 0;
}

/** int32_t getcwd(char *buf, uint32_t limit); */
int32_t
syscall_getcwd(void)
{
    char *buf;
    uint32_t limit;

    if (!sysarg_get_uint(1, &limit))
        return SYS_FAIL_RC;
    if (limit < 2)
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(0, &buf, limit))
        return SYS_FAIL_RC;

    if (!filesys_getcwd(buf, limit))
        return SYS_FAIL_RC;
    return 0;
}

/** int32_t exec(char *path, char **argv); */
int32_t
syscall_exec(void)
{
    char *path;
    uint32_t uargv;

    if (!sysarg_get_str(0, &path))
        return SYS_FAIL_RC;
    if (!sysarg_get_uint(1, &uargv))
        return SYS_FAIL_RC;

    char *argv[MAX_EXEC_ARGS];
    memset(argv, 0, MAX_EXEC_ARGS * sizeof(char *));
    for (size_t argc = 0; argc < MAX_EXEC_ARGS; ++argc) {
        uint32_t uarg;
        if (!sysarg_addr_uint(uargv + 4 * argc, &uarg))
            return SYS_FAIL_RC;
        if (uarg == 0) {    /** Reached end of list. */
            argv[argc] = 0;
            if (!filesys_exec(path, argv))
                return SYS_FAIL_RC;
            return 0;
        }
        if (sysarg_addr_str(uarg, &argv[argc]) < 0)
            return SYS_FAIL_RC;
    }
    return SYS_FAIL_RC;
}

/** int32_t fstat(int32_t fd, file_stat_t *stat); */
int32_t
syscall_fstat(void)
{
    int32_t fd;
    file_stat_t *stat;

    if (!sysarg_get_int(0, &fd))
        return SYS_FAIL_RC;
    if (!sysarg_get_mem(1, (char **) &stat, sizeof(file_stat_t)))
        return SYS_FAIL_RC;

    if (!filesys_fstat(fd, stat))
        return SYS_FAIL_RC;
    return 0;
}

/** int32_t seek(int32_t fd, uint32_t offset); */
int32_t
syscall_seek(void)
{
    int32_t fd;
    uint32_t offset;

    if (!sysarg_get_int(0, &fd))
        return SYS_FAIL_RC;
    if (!sysarg_get_uint(1, &offset))
        return SYS_FAIL_RC;

    if (!filesys_seek(fd, offset))
        return SYS_FAIL_RC;
    return 0;
}
