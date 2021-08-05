/**
 * Interrupt descriptor table (IDT) related.
 */


#ifndef IDT_H
#define IDT_H


#include <stdint.h>


/**
 * IDT gate entry format.
 * Check out https://wiki.osdev.org/IDT for detailed anatomy
 * of fields.
 */
struct idt_gate {
    uint16_t base_lo;   /** Base 0:15. */
    uint16_t selector;  /** Segment selector. */
    uint8_t  zero;      /** Unused. */
    uint8_t  flags;     /** Type & attribute flags. */
    uint16_t base_hi;   /** Base 16:31. */
} __attribute__((packed));
typedef struct idt_gate idt_gate_t;


/**
 * 48-bit IDTR address register format.
 * Used for loading the IDT with `lidt` instruction.
 */
struct idt_register {
    uint16_t boundary;  /** Boundary = length in bytes - 1. */
    uint32_t base;      /** IDT base address. */
} __attribute__((packed));
typedef struct idt_register idt_register_t;


/** Length of IDT. */
#define NUM_GATE_ENTRIES 256


void idt_init();


#endif
