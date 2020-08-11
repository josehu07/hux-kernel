/**
 * Common debugging utilities.
 */


#include <stdint.h>
#include <stddef.h>

#include "debug.h"
#include "printf.h"

#include "../boot/multiboot.h"
#include "../boot/elf.h"

#include "../common/string.h"


/** Initialized at 'debug_init'. */
static elf_symbol_t *elf_symtab;
static size_t elf_symtab_size;

static const char *elf_strtab;
static size_t elf_strtab_size;


/** Look up an address in symbols map and return its function name. */
static const char *
_lookup_symbol_name(uint32_t addr)
{
    size_t symtab_len = elf_symtab_size / sizeof(elf_symbol_t);

    for (size_t i = 0; i < symtab_len; ++i) {
        if ((ELF_SYM_TYPE(elf_symtab[i].info) == ELF_SYM_TYPE_FUNC)
            && (addr >= elf_symtab[i].value)
            && (addr <= elf_symtab[i].value + elf_symtab[i].size)) {
            return elf_strtab + elf_symtab[i].name;
        }
    }

    return "name_unresolved";
}


/**
 * Pull out the symbols table and strings table from a multiboot information
 * structure pointer.
 */
void
debug_init(multiboot_info_t *mbi)
{
    /** Get the section header table as an array of section headers. */
    elf_section_header_t *sht = (elf_section_header_t *) mbi->elf_sht.addr;
    size_t sht_len = (size_t) mbi->elf_sht.num;

    /**
     * The section header at index 'shndx' in the table is a meta section
     * header. It contains a list of section header names in the table. We
     * should look for section header names ".symtab" & ".strtab".
     */
    const char *sh_names = (const char *) sht[mbi->elf_sht.shndx].addr;

    /** Loop through the table and look for ".symtab" & ".strtab". */
    for (size_t i = 0; i < sht_len; ++i) {
        const char *name = sh_names + sht[i].name;
        if (strncmp(name, ".symtab", 7) == 0) {
            elf_symtab = (elf_symbol_t *) sht[i].addr;
            elf_symtab_size = sht[i].size;
        } else if (strncmp(name, ".strtab", 7) == 0) {
            elf_strtab = (const char *) sht[i].addr;
            elf_strtab_size = sht[i].size;
        }
    }
}


/** Print stack tracing to terminal. */
void
stack_trace()
{
    uint32_t *ebp;
    unsigned int id = 0;

    asm volatile ( "movl %%ebp, %0" : "=r" (ebp) );
    while (ebp != NULL) {
        uint32_t addr = *(ebp + 1);
        printf(" %2u) [%p] %s\n", id++, addr, _lookup_symbol_name(addr));
        ebp = (uint32_t *) *ebp;
    }
}
