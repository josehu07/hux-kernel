/**
 * User program system calls library.
 *
 * For more documentation on syscall functions signature, please see:
 * https://github.com/josehu07/hux-kernel/wiki/16.-Essential-System-Calls, and
 * https://github.com/josehu07/hux-kernel/wiki/22.-File‐Related-Syscalls
 */


#ifndef SYSCALL_H
#define SYSCALL_H


#include <stdint.h>


/** Color code for `tprint()` is in `printf.h`. */

/** Flags for `open()`. */
#define OPEN_RD 0x1
#define OPEN_WR 0x2

/** Flags for `create()`. */
#define CREATE_FILE 0x1
#define CREATE_DIR  0x2

/** Struct & type code for `fstat()`. */
#define INODE_TYPE_EMPTY 0
#define INODE_TYPE_FILE  1
#define INODE_TYPE_DIR   2

struct file_stat {
    uint32_t inumber;
    uint32_t type;
    uint32_t size;
};
typedef struct file_stat file_stat_t;

/** Struct of a directory entry, see `vsfs.h`. */
#define DENTRY_SIZE 128
#define MAX_FILENAME 100

struct dentry {
    uint32_t valid;
    uint32_t inumber;
    char filename[DENTRY_SIZE - 8];
} __attribute__((packed));
typedef struct dentry dentry_t;


/**
 * Externed from ASM `syscall.s`.
 *
 * Be sure that all arguments & returns values are 32-bit values, since
 * Hux parses syscall arguments by simply getting 32-bit values on stack.
 */
extern int32_t getpid();
extern int32_t fork(uint32_t timeslice);
extern void    exit();
extern int32_t sleep(uint32_t millisecs);
extern int32_t wait();
extern int32_t kill(int32_t pid);
extern int32_t uptime();
extern int32_t tprint(uint32_t color, char *str);
extern int32_t kbdstr(char *buf, uint32_t len);
extern int32_t setheap(uint32_t new_top);
extern int32_t open(char *path, uint32_t mode);
extern int32_t close(int32_t fd);
extern int32_t create(char *path, uint32_t mode);
extern int32_t remove(char *path);
extern int32_t read(int32_t fd, char *dst, uint32_t len);
extern int32_t write(int32_t fd, char *src, uint32_t len);
extern int32_t chdir(char *path);
extern int32_t getcwd(char *buf, uint32_t limit);
extern int32_t exec(char *path, char **argv);
extern int32_t fstat(int32_t fd, file_stat_t *stat);
extern int32_t seek(int32_t fd, uint32_t offset);
extern void    shutdown();


#endif
