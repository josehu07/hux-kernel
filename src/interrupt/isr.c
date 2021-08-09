/**
 * Interrupt service routines (ISR) handler implementation.
 */


#include <stdint.h>

#include "isr.h"
#include "idt.h"
#include "syscall.h"

#include "../common/port.h"
#include "../common/printf.h"
#include "../common/debug.h"

#include "../display/vga.h"

#include "../interrupt/syscall.h"

#include "../process/scheduler.h"


/** Table of ISRs. Unregistered entries MUST be NULL. */
isr_t isr_table[NUM_GATE_ENTRIES] = {NULL};

/** Exposed to other parts for them to register ISRs. */
inline void
isr_register(uint8_t int_no, isr_t handler)
{
    if (isr_table[int_no] != NULL) {
        error("isr: handler for interrupt # %#x already registered", int_no);
        return;
    }
    isr_table[int_no] = handler;
}


/** Send back PIC end-of-interrupt (EOI) signal. */
static void
_pic_send_eoi(uint8_t irq_no)
{
    if (irq_no >= 8)
        outb(0xA0, 0x20);   /** If is slave IRQ, should send to both. */
    outb(0x20, 0x20);
}


/** Print interrupt state information. */
static void
_print_interrupt_state(interrupt_state_t *state)
{
    info("interrupt state:");
    process_t *proc = running_proc();
    printf("  Current process: %d - %s\n", proc->pid, proc->name);
    printf("  INT#: %d  ERRCODE: %#010X  EFLAGS: %#010X\n",
           state->int_no, state->err_code, state->eflags);
    printf("  EAX: %#010X  EIP: %#010X  ESP: %#010X\n",
           state->eax, state->eip, state->esp);
    printf("   DS: %#010X   CS: %#010X   SS: %#010X\n",
           state->ds, state->cs, state->ss);
}

/**
 * Decide what to do on a missing handler:
 *   - In kernel context: we have done something wrong, panic
 *   - In user process context: user process misbehaved, exit it
 */
static void
_missing_handler(interrupt_state_t *state)
{
    _print_interrupt_state(state);
    process_t *proc = running_proc();

    bool kernel_context = (state->cs & 0x3) == 0    /** DPL field is zero. */
                          || proc == NULL;

    if (kernel_context)
        error("isr: missing handler for interrupt # %#x", state->int_no);
    else {
        warn("isr: missing handler for interrupt # %#x", state->int_no);
        process_exit();
    }
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

    /** An exception interrupt. */
    if (int_no <= 31) {

        /** Panic if no actual ISR is registered. */
        if (isr_table[int_no] == NULL)
            _missing_handler(state);
        else
            isr_table[int_no](state);

    /** An IRQ-translated interrupt from external device. */
    } else if (int_no <= 47) {
        uint8_t irq_no = state->int_no - 32;

        /** Call actual ISR if registered. */
        if (isr_table[int_no] == NULL)
            _missing_handler(state);
        else {
            /**
             * Send back timer interrupt EOI to PIC early because it may
             * yield in the timer interrupt to the scheduler, leaving
             * the PIT timer blocked for the next few processes scheduled.
             */
            if (state->int_no == INT_NO_TIMER)
                _pic_send_eoi(irq_no);
            isr_table[int_no](state);
            if (state->int_no != INT_NO_TIMER)
                _pic_send_eoi(irq_no);
        }

    /** Syscall trap. */
    } else if (int_no == INT_NO_SYSCALL) {
        process_t *proc = running_proc();

        if (proc->killed)
            process_exit();

        /** Point proc->trap_state to this trap. */
        proc->trap_state = state;

        /**
         * Call the syscall handler in `syscall.c`.
         *
         * Interrupt state contains the syscall number in EAX and the
         * arguments on the user stack. Returns an integer return code
         * back to EAX.
         */
        syscall(state);

        if (proc->killed)
            process_exit();

    /** Unknown interrupt number. */
    } else
        _missing_handler(state);
}
