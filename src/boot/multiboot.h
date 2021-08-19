/**
 * Multiboot1 related structures.
 * See https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
 * See https://www.gnu.org/software/grub/manual/multiboot/html_node/Example-OS-code.html
 */


#ifndef MULTIBOOT_H
#define MULTIBOOT_H


#include <stdint.h>


#define MULTIBOOT_HEADER_MAGIC     0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002   /** Should be in %eax. */


/**
 * Multiboot1 header.
 * See https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-layout
 */
struct multiboot_header {
    uint32_t magic;     /* Must be header magic 0x1BADB002. */
    uint32_t flags;     /* Feature flags. */
    uint32_t checksum;  /* The above fields + this one must == 0 mod 2^32. */

    /**
     * More optional fields below. Not used now, so omitted here.
     * ...
     */
} __attribute__((packed));
typedef struct multiboot_header multiboot_header_t;


/**
 * The section header table for ELF format. "These indicate where the section
 * header table from an ELF kernel is, the size of each entry, number of
 * entries, and the string table used as the index of names", stated on GRUB
 * multiboot1 specification.
 * See https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Boot-information-format
 */
struct multiboot_elf_section_header_table {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} __attribute__((packed));
typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;


/**
 * Multiboot1 information.
 * See https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Boot-information-format
 */
struct multiboot_info {
    uint32_t flags;         /* Multiboot info version number. */
    uint32_t mem_lower;     /* Available memory from BIOS. */
    uint32_t mem_upper;
    uint32_t boot_device;   /* The "root" partition. */
    uint32_t cmdline;       /* Kernel command line. */
    uint32_t mods_count;    /* Boot-Module list. */
    uint32_t mods_addr;
    
    /** We use ELF, so not including "a.out" format support. */
    multiboot_elf_section_header_table_t elf_sht;

    /**
     * More fields below. Not used now, so omitted here.
     * ...
     */
} __attribute__((packed));
typedef struct multiboot_info multiboot_info_t;


#endif
