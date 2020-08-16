/**
 * Interrupt service routines (ISR) handler implementation.
 */


#include <stdint.h>

#include "isr.h"

#include "../common/port.h"
#include "../common/printf.h"
#include "../common/debug.h"

#include "../display/vga.h"


/** Table of ISRs. Unregistered entries MUST be NULL. */
isr_t isr_table[256] = {NULL};

/** Exposed to other modules for them to register ISRs. */
inline void
isr_register(uint8_t int_no, isr_t handler)
{
    isr_table[int_no] = handler;
}


/** Send back PIC end-of-interrupt (EOI) signal. */
static void
pic_send_eoi(uint8_t irq_no)
{
    if (irq_no >= 8)
        outb(0xA0, 0x20);   /** If is slave IRQ, should send to both. */
    outb(0x20, 0x20);
}


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
    uint8_t int_no = state->int_no;

    /** A trap gate interrupt, i.e., an exception. */
    if (int_no <= 31) {

        /** Panic if no actual ISR is registered. */
        if (isr_table[int_no] == NULL)
            error("missing handler for interrupt # %#x", int_no);
        else
            isr_table[int_no](state);

    /** An IRQ-translated interrupt from external device. */
    } else if (int_no <= 47) {
        uint8_t irq_no = state->int_no - 32;

        /** Call actual ISR if registered. */
        if (isr_table[int_no] != NULL)
            isr_table[int_no](state);

        pic_send_eoi(irq_no);   /** Remember to send back EOI signal. */
    }
}
