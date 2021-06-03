/**
 * Interrupt service routines (ISR) handler implementation.
 */


#ifndef ISR_H
#define ISR_H


#include <stdint.h>


/**
 * Interrupt state specification, which will be followed by `isr-stub.s`
 * before calling `isr_handler` below.
 */
struct interrupt_state {
    uint32_t ds;
    uint32_t edi, esi, ebp, useless, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, esp, ss;
} __attribute__((packed));
typedef struct interrupt_state interrupt_state_t;


/** Allow other modules to register an ISR. */
typedef void (*isr_t)(interrupt_state_t *);

void isr_register(uint8_t int_no, isr_t handler);


#endif
