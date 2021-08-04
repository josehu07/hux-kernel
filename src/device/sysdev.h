/**
 * Syscalls related to communication with external devices other than
 * the VGA terminal display.
 */


#ifndef SYSDEV_H
#define SYSDEV_H


#include <stdint.h>


int32_t syscall_uptime();
int32_t syscall_kbdstr();


#endif
