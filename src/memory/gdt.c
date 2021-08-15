/**
 * Global descriptor table (GDT) related.
 */


#include <stdint.h>
#include <stddef.h>

#include "gdt.h"

#include "../common/debug.h"

#include "../process/process.h"


/**
 * The GDT table. We maintain 6 entries:
 *   0. a null entry;
 *   1. kernel mode code segment;
 *   2. kernel mode data segment;
 *   3. user mode code segment;
 *   4. user mode data segment;
 *   5. task state segment for user mode execution.
 */
static gdt_entry_t gdt[NUM_SEGMENTS];

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


/** Extern our load routine written in ASM `gdt-load.s`. */
extern void gdt_load(uint32_t gdtr_ptr, uint32_t data_selector_offset,
                                        uint32_t code_selector_offset);


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
    gdt_set_entry(SEGMENT_UNUSED, 0u, 0u, 0u, 0u);  /** 0-th entry unused. */
    gdt_set_entry(SEGMENT_KCODE, 0u, 0xFFFFF, 0x9A, 0xC0);
    gdt_set_entry(SEGMENT_KDATA, 0u, 0xFFFFF, 0x92, 0xC0);
    gdt_set_entry(SEGMENT_UCODE, 0u, 0xFFFFF, 0xFA, 0xC0);
    gdt_set_entry(SEGMENT_UDATA, 0u, 0xFFFFF, 0xF2, 0xC0);

    /** Setup the GDTR register value. */
    gdtr.boundary = (sizeof(gdt_entry_t) * NUM_SEGMENTS) - 1;
    gdtr.base     = (uint32_t) &gdt;

    /**
     * Load the GDT.
     * Passing pointer to `gdtr` as unsigned integer. Each GDT entry takes 8
     * bytes, therefore kernel data selector is at 0x10 and kernel code
     * selector is at 0x08.
     */
    gdt_load((uint32_t) &gdtr, SEGMENT_KDATA << 3, SEGMENT_KCODE << 3);
}


/**
 * Set up TSS for a process to be switched, so that the CPU will be able
 * to jump to its kernel stack when a system call happens.
 * Check out https://wiki.osdev.org/Task_State_Segment for details.
 *
 * Must be called with `cli` pushed explicitly.
 */
void
gdt_switch_tss(tss_t *tss, process_t *proc)
{
    assert(proc != NULL);
    assert(proc->pgdir != NULL);
    assert(proc->kstack != 0);

    /**
     * Task state segment (TSS) has:
     *
     * Access Byte -
     *   - Pr    = 1: present
     *   - Privl = 0: kernel privilege
     *   - S     = 0: it is a system segment
     *   - Ex    = 1: executable
     *   - DC    = 0: conforming
     *   - RW    = 0: readable code
     *   - Ac    = 1: accessed
     *   Hence, 0x89.
     */
    gdt_set_entry(5, (uint32_t) tss, (uint32_t) (sizeof(tss_t) - 1),
                  0x89, 0x00);

    /** Fill in task state information. */
    tss->ss0 = SEGMENT_KDATA << 3;              /** Kernel data segment. */
    tss->esp0 = proc->kstack + KSTACK_SIZE;     /** Top of kernel stack. */
    tss->iopb = sizeof(tss_t);  /** Forbids e.g. inb/outb from user space. */
    tss->ebp = 0;   /** Ensure EBP is 0 on switch, for stack backtracing. */

    /**
     * Load task segment register. Segment selectors need to be shifted
     * to the left by 3, because the lower 3 bits are TI & RPL flags.
     */
    uint16_t tss_seg_reg = SEGMENT_TSS << 3;
    asm volatile ( "ltr %0" : : "r" (tss_seg_reg) );
}
