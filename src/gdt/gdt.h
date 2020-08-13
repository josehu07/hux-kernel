/**
 * Global descriptor table (GDT) related.
 */


#ifndef GDT_H
#define GDT_H


#include <stdint.h>


/**
 * GDT entry format.
 * Check out https://wiki.osdev.org/Global_Descriptor_Table
 * for detailed anatomy of fields.
 */
struct gdt_entry
{
    uint16_t limit_lo;          /** Limit 15:0. */
    uint16_t base_lo;           /** Base 15:0. */
    uint8_t  base_mi;           /** Base 23:16. */
    uint8_t  access;            /** Access Byte. */
    uint8_t  limit_hi_flags;    /** Limit 16:19 | FLags. */
    uint8_t  base_hi;           /** Base 24:31. */
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;


/**
 * 48-bit GDTR address register format.
 * Used for loading the GDT table with `lgdt` instruction.
 */
struct gdt_register
{
    uint16_t boundary;  /** Boundary = length in bytes - 1. */
    uint32_t base;      /** GDT base address. */
} __attribute__((packed));
typedef struct gdt_register gdt_register_t;


void gdt_init();


#endif
