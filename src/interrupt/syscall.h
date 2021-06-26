/**
 * System call-related definitions and handler wrappers.
 */


#ifndef SYSCALL_H
#define SYSCALL_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "isr.h"


/** Syscall trap gate registerd at a vacant ISR number. */
#define INT_NO_SYSCALL 64   /** == 0x40 */

/** List of known syscall numbers. */
#define SYSCALL_HELLO 1


/**
 * Task state segment (TSS) x86 IA32 format,
 * see https://wiki.osdev.org/Task_State_Segment#x86_Structure.
 */
struct task_state_segment {
    uint32_t link;      /** Old TS selector. */
    uint32_t esp0;      /** Stack pointer after privilege level boost. */
    uint8_t  ss0;       /** Segment selector after privilege level boost. */
    uint8_t  pad1;
    uint32_t esp1;
    uint8_t  ss1;
    uint8_t  pad2;
    uint32_t esp2;
    uint8_t  ss2;
    uint8_t  pad3;
    uint32_t cr3;       /** Page directory base address. */
    uint32_t eip;       /** Saved EIP from last task switch. Same for below. */
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint8_t  es;
    uint8_t  pad4;
    uint8_t  cs;
    uint8_t  pad5;
    uint8_t  ss;
    uint8_t  pad6;
    uint8_t  ds;
    uint8_t  pad7;
    uint8_t  fs;
    uint8_t  pad8;
    uint8_t  gs;
    uint8_t  pad9;
    uint8_t  ldt;
    uint8_t  pad10;
    uint8_t  pad11;
    uint8_t  iopb;       /** I/O map base address. */
} __attribute__((packed));
typedef struct task_state_segment tss_t;


/** Individual syscall handler type: void -> int32_t. */
typedef int32_t (*syscall_t)(void);

/** Syscall unsuccessful return code. */
#define SYS_FAIL_RC (-1)

void syscall(interrupt_state_t *state);


bool sysarg_get_int(int8_t n, int32_t *ret);
bool sysarg_get_mem(int8_t n, char **mem, size_t len);
int32_t sysarg_get_str(int8_t n, char **str);


#endif
