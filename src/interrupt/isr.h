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


/** Allow other parts to register an ISR. */
typedef void (*isr_t)(interrupt_state_t *);

void isr_register(uint8_t int_no, isr_t handler);


/**
 * List of known interrupt numbers in this system. Other parts of the kernel
 * should refer to these macro names instead of using plain numbers.
 *   - 0 - 31 are ISRs for CPU-generated exceptions, processor-defined,
 *     see https://wiki.osdev.org/Interrupt_Vector_Table
 *   - 32 - 47 are mapped as custom device IRQs, so ISR 32 means IRQ 0, etc.
 *   - 64 i.e. 0x40 is chosen as our syscall trap gate
 */
#define INT_NO_DIV_BY_ZERO      0   /** Divide by zero. */
//                              1   /** Reserved. */
#define INT_NO_NMI              2   /** Non maskable interrupt (NMI). */
#define INT_NO_BREAKPOINT       3   /** Breakpoint. */
#define INT_NO_OVERFLOW         4   /** Overflow. */
#define INT_NO_BOUNDS           5   /** Bounds range exceeded. */
#define INT_NO_ILLEGAL_OP       6   /** Illegal opcode. */
#define INT_NO_DEVICE_NA        7   /** Device not available. */
#define INT_NO_DOUBLE_FAULT     8   /** Double fault. */
//                              9   /** No longer used. */
#define INT_NO_INVALID_TSS      10  /** Invalid task state segment (TSS). */
#define INT_NO_SEGMENT_NP       11  /** Segment not present. */
#define INT_NO_STACK_SEG        12  /** Stack segment fault. */
#define INI_NO_PROTECTION       13  /** General protection fault. */
#define INT_NO_PAGE_FAULT       14  /** Page fault. */
//                              15  /** Reserved. */
#define INT_NO_FPU_ERROR        16  /** Floating-point unit (FPU) error. */
#define INT_NO_ALIGNMENT        17  /** Alignment check */
#define INT_NO_MACHINE          18  /** Machine check. */
#define INT_NO_SIMD_FP          19  /** SIMD floating-point error. */
//                         20 - 31  /** Reserved. */

#define IRQ_BASE_NO     32
#define INT_NO_TIMER    (IRQ_BASE_NO + 0)
#define INT_NO_KEYBOARD (IRQ_BASE_NO + 1)
#define INT_NO_IDEDISK  (IRQ_BASE_NO + 14)

/** INT_NO_SYSCALL is 64, defined in `syscall.h`. */


#endif
