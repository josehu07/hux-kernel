/**
 * Interrupt descriptor table (IDT) related.
 */


#include <stdint.h>

#include "idt.h"

#include "../common/string.h"


/**
 * The IDT table. Should contain 256 gate entries.
 *   -  0 -  31: reserved by x86 CPU for various exceptions
 *   - 32 - 255: free for our OS kernel to define
 */
static idt_gate_t idt[256];

/** IDTR address register. */
static idt_register_t idtr;


/**
 * Setup one IDT gate entry.
 * Here, BASE is in its complete version, SELECTOR represents the selector
 * field, and FLAGS represents the 8-bit flags field.
 */
static void
idt_set_gate(int idx, uint32_t base, uint16_t selector, uint8_t flags)
{
    idt[idx].base_lo  = (uint16_t) (base & 0xFFFF);
    idt[idx].selector = (uint16_t) selector;
    idt[idx].zero     = (uint8_t) 0;
    idt[idx].flags    = (uint8_t) flags;
    idt[idx].base_hi  = (uint16_t) ((base >> 16) & 0xFFFF);
}


/** Extern our load routine written in ASM `idt-load.s`. */
extern void idt_load(uint32_t idtr_ptr);


/** Extern our trap ISR handlers written in ASM `isr-stub.s`. */
extern void isr0 (void);
extern void isr1 (void);
extern void isr2 (void);
extern void isr3 (void);
extern void isr4 (void);
extern void isr5 (void);
extern void isr6 (void);
extern void isr7 (void);
extern void isr8 (void);
extern void isr9 (void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);


/**
 * Initialize the interrupt descriptor table (IDT) by setting up gate
 * entries of IDT, setting the IDTR register to point to our IDT address,
 * and then (through assembly `lidt` instruction) load our IDT.
 */
void
idt_init()
{
    /**
     * First, see https://wiki.osdev.org/IDT for a detailed anatomy of
     * flags field.
     *
     * Flags -
     *   - P    = ?: present, 0 for inactive entries and 1 for valid ones
     *   - DPL  = ?: ring level, specifies which privilege level should the
     *               calling segment at least have
     *   - S    = ?: 0 for interrupt and trap gates
     *   - Type =
     *     - 0x5: 32-bit task gate
     *     - 0x6: 16-bit interrupt gate
     *     - 0x7: 16-bit trap gate
     *     - 0xE: 32-bit interrupt gate
     *     - 0xF: 32-bit trap gate
     * Hence, all interrupt gates have flag field 0x8E and all trap gates
     * have flag field 0x8F for now. The difference between trap gates and
     * interrupt gates is that interrupt gates automatically disable
     * interrupts upon entry and restores upon `iret` instruction (which
     * restores the saved EFLAGS). Trap gates do not do this.
     * 
     * Selector = 0x08: pointing to kernel code segment.
     *
     * Unused entries and field all default to 0, so memset first.
     */
    memset(idt, 0, sizeof(idt_gate_t) * 256);

    idt_set_gate(0 , (uint32_t) isr0 , 0x08, 0x8F);
    idt_set_gate(1 , (uint32_t) isr1 , 0x08, 0x8F);
    idt_set_gate(2 , (uint32_t) isr2 , 0x08, 0x8F);
    idt_set_gate(3 , (uint32_t) isr3 , 0x08, 0x8F);
    idt_set_gate(4 , (uint32_t) isr4 , 0x08, 0x8F);
    idt_set_gate(5 , (uint32_t) isr5 , 0x08, 0x8F);
    idt_set_gate(6 , (uint32_t) isr6 , 0x08, 0x8F);
    idt_set_gate(7 , (uint32_t) isr7 , 0x08, 0x8F);
    idt_set_gate(8 , (uint32_t) isr8 , 0x08, 0x8F);
    idt_set_gate(9 , (uint32_t) isr9 , 0x08, 0x8F);
    idt_set_gate(10, (uint32_t) isr10, 0x08, 0x8F);
    idt_set_gate(11, (uint32_t) isr11, 0x08, 0x8F);
    idt_set_gate(12, (uint32_t) isr12, 0x08, 0x8F);
    idt_set_gate(13, (uint32_t) isr13, 0x08, 0x8F);
    idt_set_gate(14, (uint32_t) isr14, 0x08, 0x8F);
    idt_set_gate(15, (uint32_t) isr15, 0x08, 0x8F);
    idt_set_gate(16, (uint32_t) isr16, 0x08, 0x8F);
    idt_set_gate(17, (uint32_t) isr17, 0x08, 0x8F);
    idt_set_gate(18, (uint32_t) isr18, 0x08, 0x8F);
    idt_set_gate(19, (uint32_t) isr19, 0x08, 0x8F);
    idt_set_gate(20, (uint32_t) isr20, 0x08, 0x8F);
    idt_set_gate(21, (uint32_t) isr21, 0x08, 0x8F);
    idt_set_gate(22, (uint32_t) isr22, 0x08, 0x8F);
    idt_set_gate(23, (uint32_t) isr23, 0x08, 0x8F);
    idt_set_gate(24, (uint32_t) isr24, 0x08, 0x8F);
    idt_set_gate(25, (uint32_t) isr25, 0x08, 0x8F);
    idt_set_gate(26, (uint32_t) isr26, 0x08, 0x8F);
    idt_set_gate(27, (uint32_t) isr27, 0x08, 0x8F);
    idt_set_gate(28, (uint32_t) isr28, 0x08, 0x8F);
    idt_set_gate(29, (uint32_t) isr29, 0x08, 0x8F);
    idt_set_gate(30, (uint32_t) isr30, 0x08, 0x8F);
    idt_set_gate(31, (uint32_t) isr31, 0x08, 0x8F);

    /** Setup the IDTR register value. */
    idtr.boundary = (sizeof(idt_gate_t) * 256) - 1;     /** Length - 1. */
    idtr.base     = (uint32_t) &idt;

    /**
     * Load the IDT.
     * Passing pointer to `idtr` as unsigned integer.
     */
    idt_load((uint32_t) &idtr);
}
