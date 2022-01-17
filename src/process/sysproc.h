/**
 * Syscalls related to process state & operations.
 */


#ifndef SYSPROC_H
#define SYSPROC_H


#include <stdint.h>


int32_t syscall_getpid();
int32_t syscall_fork();
int32_t syscall_exit();
int32_t syscall_sleep();
int32_t syscall_wait();
int32_t syscall_kill();
int32_t syscall_shutdown();


#endif
