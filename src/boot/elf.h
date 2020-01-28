/**
 * ELF format related structures.
 * See http://www.cs.cmu.edu/afs/cs/academic/class/15213-f00/docs/elf.pdf.
 */


#ifndef ELF_H
#define ELF_H


#include <stdint.h>


/**
 * For getting the type of a symbol table entry. Function type code == 0x2.
 * See https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-79797.html#chapter6-tbl-21.
 */
#define ELF_SYM_TYPE(info) ((info) & 0xf)
#define ELF_SYM_TYPE_FUNC  0x2


/** ELF section header. */
struct elf_section_header {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} __attribute__((packed));
typedef struct elf_section_header elf_section_header_t;


/** ELF symbol. */
struct elf_symbol {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t  info;
    uint8_t  other;
    uint16_t shndx;
} __attribute__((packed));
typedef struct elf_symbol elf_symbol_t;


#endif
