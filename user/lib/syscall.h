/**
 * User program system calls library.
 *
 * For more documentation on syscall functions signature, please see:
 * https://github.com/josehu07/hux-kernel/wiki/16.-Essential-System-Calls
 */


#ifndef SYSCALL_H
#define SYSCALL_H


#include <stdint.h>

#include "printf.h"


/** Externed from ASM `syscall.s`. */
extern int8_t getpid();
extern int8_t fork();
extern void exit();
extern int8_t sleep(uint32_t millisecs);
extern int8_t wait();
extern int8_t kill(int8_t pid);
extern void tprint(vga_color_t color, char *str);
extern int32_t uptime();
extern int32_t kbdstr();
extern int8_t setheap(uint32_t new_top);


#endif
