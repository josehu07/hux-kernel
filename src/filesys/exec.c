/**
 * Implementation of the `exec()` syscall on ELF-32 file.
 */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "exec.h"
#include "file.h"
#include "vsfs.h"

#include "../boot/elf.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../memory/paging.h"
#include "../memory/slabs.h"

#include "../process/process.h"
#include "../process/scheduler.h"
#include "../process/layout.h"


/**
 * Refresh page table, load an executable ELF program at given inode, and
 * start execution at the beginning of its text section. ARGV is an array
 * of strings (`char *`s) where the last one must be NULL, indicating
 * the end of argument array.
 *
 * The syscall does not actually return on success, since the process
 * should have jumped to the newly loaded code after returning from this
 * trap frame. Returns false on failures.
 */
bool
exec_program(mem_inode_t *inode, char *filename, char **argv)
{
    process_t *proc = running_proc();
    pde_t *pgdir = NULL;

    inode_lock(inode);

    /** Read in ELF header, sanity check magic number. */
    elf_file_header_t elf_header;
    if (inode_read(inode, (char *) &elf_header, 0,
                   sizeof(elf_file_header_t)) != sizeof(elf_file_header_t)) {
        warn("exec: failed to read ELF file header");
        goto fail;
    }
    if (elf_header.magic != ELF_MAGIC) {
        warn("exec: ELF header magic number mismatch");
        goto fail;
    }

    /**
     * Mimicks `initproc_init()` in `process.c`. Sets up a brand-new page table
     * and pre-maps necessary pages:
     *   - kernel mapped to lower 512MiB
     *   - program ELF binary follows
     *   - top-most stack page
     *
     * Need to set up a brand-new copy of page table because if there are any
     * errors that occur during the process, we can gracefully return an error
     * to the caller process instead of breaking it.
     */
    pgdir = (pde_t *) salloc_page();
    if (pgdir == NULL) {
        warn("exec: failed to allocate new page directory");
        goto fail;
    }
    memset(pgdir, 0, sizeof(pde_t) * PDES_PER_PAGE);

    uint32_t vaddr_btm = 0;                     /** Kernel-mapped. */
    while (vaddr_btm < PHYS_MAX) {
        pte_t *pte = paging_walk_pgdir(pgdir, vaddr_btm, true);
        if (pte == NULL)
            goto fail;
        paging_map_kpage(pte, vaddr_btm);

        vaddr_btm += PAGE_SIZE;
    }
    
    elf_program_header_t prog_header;           /** ELF binary. */
    uint32_t vaddr_elf_max = USER_BASE;
    for (size_t idx = 0; idx < elf_header.phnum; ++idx) {
        /** Read in this program header. */
        size_t offset = elf_header.phoff + idx * sizeof(elf_program_header_t);
        if (inode_read(inode, (char *) &prog_header, offset,
                       sizeof(elf_program_header_t)) != sizeof(elf_program_header_t)) {
            goto fail;
        }
        if (prog_header.type != ELF_PROG_TYPE_LOAD)
            continue;
        if (prog_header.memsz < prog_header.filesz)
            goto fail;

        /** Read in program segment described by this header. */
        uint32_t vaddr_curr = prog_header.vaddr;
        uint32_t vaddr_end = prog_header.vaddr + prog_header.memsz;
        uint32_t elf_curr = prog_header.offset;
        uint32_t elf_end = prog_header.offset + prog_header.filesz;
        while (vaddr_curr < vaddr_end) {
            size_t effective_v = PAGE_SIZE - ADDR_PAGE_OFFSET(vaddr_curr);
            if (effective_v > vaddr_end - vaddr_curr)
                effective_v = vaddr_end - vaddr_curr;
            size_t effective_e = effective_v;
            if (effective_e > elf_end - elf_curr)
                effective_e = elf_end - elf_curr;

            if (vaddr_curr < USER_BASE) {
                vaddr_curr += effective_v;
                elf_curr += effective_e;
                continue;
            }

            pte_t *pte = paging_walk_pgdir(pgdir, vaddr_curr, true);
            if (pte == NULL)
                goto fail;
            uint32_t paddr = pte->present == 0 ? paging_map_upage(pte, true)
                                               : ENTRY_FRAME_ADDR(*pte);
            if (paddr == 0)
                goto fail;
            uint32_t paddr_curr = paddr + ADDR_PAGE_OFFSET(vaddr_curr);

            if (effective_e > 0) {
                if (inode_read(inode, (char *) paddr_curr, elf_curr,
                               effective_e) != effective_e) {
                    goto fail;
                }
                elf_curr += effective_e;
            }

            vaddr_curr += effective_v;
        }

        if (vaddr_curr > vaddr_elf_max)
            vaddr_elf_max = ADDR_PAGE_ROUND_UP(vaddr_curr);
    }

    inode_unlock(inode);
    inode_put(inode);
    inode = NULL;

    while (vaddr_elf_max < HEAP_BASE) {         /** Rest of ELF region. */
        pte_t *pte = paging_walk_pgdir(pgdir, vaddr_elf_max, true);
        if (pte == NULL)
            goto fail;
        uint32_t paddr = paging_map_upage(pte, true);
        if (paddr == 0)
            goto fail;

        vaddr_elf_max += PAGE_SIZE;
    }
    
    uint32_t vaddr_top = USER_MAX - PAGE_SIZE;  /** Top stack page. */
    pte_t *pte_top = paging_walk_pgdir(pgdir, vaddr_top, true);
    if (pte_top == NULL)
        goto fail;
    uint32_t paddr_top = paging_map_upage(pte_top, true);
    if (paddr_top == 0)
        goto fail;
    memset((char *) paddr_top, 0, PAGE_SIZE);

    /**
     * Push argument strings to the stack, then push the argv list
     * pointing to those strings, followed by `argv`, `argc`.
     */
    uint32_t sp = USER_MAX;
    uint32_t ustack[3 + MAX_EXEC_ARGS + 1];
    size_t argc = 0;
    for (argc = 0; argv[argc] != NULL; ++argc) {
        if (argc >= MAX_EXEC_ARGS)
            goto fail;
        sp = sp - (strlen(argv[argc]) + 1);
        sp &= 0xFFFFFFFC;   /** Align to 32-bit words. */
        memcpy((char *) (paddr_top + PAGE_SIZE - (USER_MAX - sp)), argv[argc],
               strlen(argv[argc]) + 1);
        ustack[3 + argc] = sp;
    }
    ustack[3 + argc] = 0;       /** End of argv list. */

    ustack[2] = sp - (argc + 1) * 4;       /** `argv` */
    ustack[1] = argc;                      /** `argc` */
    ustack[0] = 0x0000DEAD;  /** Fake return address. */

    sp -= (3 + argc + 1) * 4;
    memcpy((char *) (paddr_top + PAGE_SIZE - (USER_MAX - sp)), ustack,
           (3 + argc + 1) * 4);

    /** Change process name. */
    strncpy(proc->name, filename, strlen(filename));

    /** Switch to the new page directory, discarding old state. */
    pde_t *old_pgdir = proc->pgdir;
    uint32_t old_heap_high = proc->heap_high;
    uint32_t old_stack_low = proc->stack_low;

    proc->pgdir = pgdir;
    proc->stack_low = vaddr_top;
    proc->heap_high = HEAP_BASE;
    proc->trap_state->esp = sp;
    proc->trap_state->eip = elf_header.entry;   /** `main()` function. */
    paging_switch_pgdir(proc->pgdir);

    paging_unmap_range(old_pgdir, USER_BASE, old_heap_high);
    paging_unmap_range(old_pgdir, old_stack_low, USER_MAX);
    paging_destroy_pgdir(old_pgdir);
    return true;

fail:
    if (pgdir != NULL) {
        paging_unmap_range(pgdir, USER_BASE, HEAP_BASE);
        paging_unmap_range(pgdir, USER_MAX - PAGE_SIZE, USER_MAX);
        paging_destroy_pgdir(pgdir);
    }
    if (inode != NULL) {
        inode_unlock(inode);
        inode_put(inode);
    }
    return false;
}
