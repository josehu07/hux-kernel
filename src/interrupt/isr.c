/**
 * Interrupt descriptor table (IDT) related.
 */


#include <stdint.h>

#include "../common/printf.h"
#include "../common/debug.h"

#include "../display/vga.h"


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


/**
 * ISR handler written in C.
 *
 * Receives a pointer to a structure of interrupt state. Handles the
 * interrupt and simply returns. Can modify interrupt state through
 * this pointer if necesary.
 */
void
isr_handler(interrupt_state_t *state)
{
    info("caught interrupt # %#x\n", state->int_no);
    /** Currently unimplemented. */
}
