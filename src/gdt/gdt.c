/**
 * Global descriptor table (GDT) related.
 */


#include <stdint.h>

#include "gdt.h"


/**
 * The GDT table. We maintain 5 entries:
 *   0. a null entry;
 *   1. kernel mode code segment;
 *   2. kernel mode data segment;
 *   3. user mode code segment;
 *   4. user mode data segment;
 */
static gdt_entry_t gdt[5];

/** GDTR address register. */
static gdt_register_t gdtr;


/**
 * Setup one GDT entry.
 * Here, BASE and LIMIT are in their complete version, ACCESS represents
 * the access byte, and FLAGS has the 4-bit granularity flags field in its
 * higer 4 bits.
 */
static void
gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access,
              uint8_t flags)
{
    gdt[idx].limit_lo       =  (uint16_t) (limit & 0xFFFF);
    gdt[idx].base_lo        =  (uint16_t) (base & 0xFFFF);
    gdt[idx].base_mi        =  (uint8_t) ((base >> 16) & 0xFF);
    gdt[idx].access         =  (uint8_t) access;
    gdt[idx].limit_hi_flags =  (uint8_t) ((limit >> 16) & 0x0F);
    gdt[idx].limit_hi_flags |= (uint8_t) (flags & 0xF0);
    gdt[idx].base_hi        =  (uint8_t) ((base >> 24) & 0xFF);
}


/** Extern our load routine written in ASM `load.s`. */
extern void gdt_load(uint32_t gdtr_ptr);


/**
 * Initialize the global descriptor table (GDT) by setting up the 5 entries
 * of GDT, setting the GDTR register to point to our GDT address, and then
 * (through assembly `lgdt` instruction) load our GDT.
 */
void
gdt_init()
{
    /**
     * First, see https://wiki.osdev.org/Global_Descriptor_Table for a
     * detailed anatomy of Access Byte and Flags fields.
     * 
     * Access Byte -
     *   - Pr    = 1: present, must be 1 for valid selectors
     *   - Privl = ?: ring level, 0 for kernel and 3 for user mode
     *   - S     = 1: should be 1 for all non-system segments
     *   - Ex    = ?: executable, 1 for code and 0 for data segment
     *   - DC    =
     *     - Direction bit for data selectors,  = 0: segment spans up
     *     - Conforming bit for code selectors, = 0: can only be executed
     *                                               from ring level set
     *                                               in `Privl` field
     *   - RW    =
     *     - Readable bit for code selectors, = 1: allow reading
     *     - Writable bit for data selectors, = 1: allow writing
     *   - Ac    = 0: access bit, CPU sets it to 1 when accessing it
     *   Hence, the four values used below.
     * 
     * Flags -
     *   - Gr = 1: using page granularity
     *   - Sz = 1: in 32-bit protected mode
     *   Hence, 0b1100 -> 0xC for all these four segments. 
     */
    gdt_set_entry(0, 0u, 0u, 0u, 0u);
    gdt_set_entry(1, 0u, 0xFFFFF, 0x9A, 0xC0);  /** Kernel code segment. */
    gdt_set_entry(2, 0u, 0xFFFFF, 0x92, 0xC0);  /** Kernel data segment. */
    gdt_set_entry(3, 0u, 0xFFFFF, 0xFA, 0xC0);  /** User mode code segment. */
    gdt_set_entry(3, 0u, 0xFFFFF, 0xF2, 0xC0);  /** User mode data segment. */

    /** Setup the GDTR register value. */
    gdtr.boundary = (sizeof(gdt_entry_t) * 5) - 1;  /** Length - 1. */
    gdtr.base     = (uint32_t) &gdt;

    /** Load the GDT. (Passing pointer to `gdtr` as unsigned.) */
    gdt_load((uint32_t) &gdtr);
}
