/**
 * Global descriptor table (GDT) related.
 */


#ifndef GDT_H
#define GDT_H


#include <stdint.h>

#include "../process/process.h"
#include "../process/scheduler.h"


/**
 * GDT entry format.
 * Check out https://wiki.osdev.org/Global_Descriptor_Table
 * for detailed anatomy of fields.
 */
struct gdt_entry {
    uint16_t limit_lo;          /** Limit 0:15. */
    uint16_t base_lo;           /** Base 0:15. */
    uint8_t  base_mi;           /** Base 16:23. */
    uint8_t  access;            /** Access Byte. */
    uint8_t  limit_hi_flags;    /** Limit 16:19 | FLags. */
    uint8_t  base_hi;           /** Base 24:31. */
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;


/**
 * 48-bit GDTR address register format.
 * Used for loading the GDT table with `lgdt` instruction.
 */
struct gdt_register {
    uint16_t boundary;  /** Boundary = length in bytes - 1. */
    uint32_t base;      /** GDT base address. */
} __attribute__((packed));
typedef struct gdt_register gdt_register_t;


/** List of segments registered in GDT. */
#define SEGMENT_UNUSED 0x0
#define SEGMENT_KCODE  0x1
#define SEGMENT_KDATA  0x2
#define SEGMENT_UCODE  0x3
#define SEGMENT_UDATA  0x4
#define SEGMENT_TSS    0x5

#define NUM_SEGMENTS 6


void gdt_init();

void gdt_switch_tss(tss_t *tss, process_t *proc);


#endif
