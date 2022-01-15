/**
 * Syscalls related to process state & operations.
 */


#ifndef SYSFILE_H
#define SYSFILE_H


#include <stdint.h>


/** Flags for `open()`. */
#define OPEN_RD 0x1
#define OPEN_WR 0x2

/** Flags for `create()`. */
#define CREATE_FILE 0x1
#define CREATE_DIR  0x2


/** For the `fstat()` syscall. */
struct file_stat {
    uint32_t inumber;
    uint32_t type;
    uint32_t size;
};
typedef struct file_stat file_stat_t;


int32_t syscall_open();
int32_t syscall_close();
int32_t syscall_create();
int32_t syscall_remove();
int32_t syscall_read();
int32_t syscall_write();
int32_t syscall_chdir();
int32_t syscall_getcwd();
int32_t syscall_exec();
int32_t syscall_fstat();
int32_t syscall_seek();


#endif
