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


#endif
