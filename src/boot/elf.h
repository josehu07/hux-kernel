/**
 * ELF 32-bit format related structures.
 * See http://www.cs.cmu.edu/afs/cs/academic/class/15213-f00/docs/elf.pdf
 */


#ifndef ELF_H
#define ELF_H


#include <stdint.h>


/** ELF file header. */
struct elf_file_header {
    uint32_t magic;         /** In little-endian on x86. */
    uint8_t  ident[12];     /** Rest of `e_ident`. */
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__((packed));
typedef struct elf_file_header elf_file_header_t;

/**
 * ELF magic number 0x7F'E''L''F' in little endian.
 * See https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html#elfid
 */
#define ELF_MAGIC 0x464C457F


/** ELF program header. */
struct elf_program_header {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} __attribute__((packed));
typedef struct elf_program_header elf_program_header_t;

/** ELF program header flags. */
#define ELF_PROG_FLAG_EXEC  0x1
#define ELF_PROG_FLAG_WRITE 0x2
#define ELF_PROG_FLAG_READ  0x4

/** ELF program header types. */
#define ELF_PROG_TYPE_LOAD  0x1


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

/**
 * For getting the type of a symbol table entry. Function type code == 0x2.
 * See https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-79797.html#chapter6-tbl-21
 */
#define ELF_SYM_TYPE(info) ((info) & 0xf)
#define ELF_SYM_TYPE_FUNC  0x2


#endif
