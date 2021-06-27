/**
 * Setting up and switching to paging mode.
 */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "paging.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../interrupt/isr.h"

#include "../memory/slabs.h"


/** Kernel heap bottom address - should be above `elf_shstrtab_end`. */
uint32_t kheap_curr;

/** kernel's identity-mapping page directory. */
pde_t *kernel_pgdir;    /** Allocated at paging init. */

/** Bitmap indicating free/used frames. */
static uint32_t *frame_bitmap;


/**
 * Auxiliary function for allocating (page-aligned) chunks of memory in the
 * kernel heap region that never gets freed.
 * 
 * Should only be used to allocate the kernel's page directory/tables and
 * the frames bitmap and other things before our actual heap allocation
 * algorithm setup.
 */
static uint32_t
_kalloc_temp(size_t size, bool page_align)
{
    /** If `page_align` is set, return an aligned address. */
    if (page_align && !ADDR_PAGE_ALIGNED(kheap_curr))
        kheap_curr = ADDR_PAGE_ROUND_UP(kheap_curr);

    /** If exceeds the 8MiB kernel memory boundary, panic. */
    if (kheap_curr + size > KMEM_MAX)
        error("_kalloc_temp: kernel memory exceeds boundary");

    uint32_t temp = kheap_curr;
    kheap_curr += size;
    return temp;
}


/**
 * Helper functions for managing free physical frames, using a bitmap
 * data structure. Every bit indicates the free/used state of a corresponding
 * physical frame. Frame number one-one maps to bit index.
 */
#define BITMAP_OUTER_IDX(frame_num) ((frame_num) / 32)
#define BITMAP_INNER_IDX(frame_num) ((frame_num) % 32)

/** Set a frame as used. */
static inline void
frame_bitmap_set(uint32_t frame_num)
{
    size_t outer_idx = BITMAP_OUTER_IDX(frame_num);
    size_t inner_idx = BITMAP_INNER_IDX(frame_num);
    frame_bitmap[outer_idx] |= (1 << inner_idx);
}

/** Clear a frame as free. */
static inline void
frame_bitmap_clear(uint32_t frame_num)
{
    size_t outer_idx = BITMAP_OUTER_IDX(frame_num);
    size_t inner_idx = BITMAP_INNER_IDX(frame_num);
    frame_bitmap[outer_idx] &= ~(1 << inner_idx);
}

/** Returns true if a frame is in use, otherwise false. */
static inline bool
frame_bitmap_check(uint32_t frame_num)
{
    size_t outer_idx = BITMAP_OUTER_IDX(frame_num);
    size_t inner_idx = BITMAP_INNER_IDX(frame_num);
    return frame_bitmap[outer_idx] & (1 << inner_idx);
}

/**
 * Allocate a frame and mark as used. Returns the frame number of
 * the allocated frame, or panics if there is no free frame.
 */
static uint32_t
frame_bitmap_alloc(void)
{
    for (size_t i = 0; i < (NUM_FRAMES / 32); ++i) {
        if (frame_bitmap[i] == 0xFFFFFFFF)
            continue;
        for (size_t j = 0; j < 32; ++j) {
            if ((frame_bitmap[i] & (1 << j)) == 0) {
                /** Found a free frame. */
                uint32_t frame_num = i * 32 + j;
                frame_bitmap_set(frame_num);
                return i * 32 + j;
            }
        }
    }

    return NUM_FRAMES;
}


/**
 * Helper that allocates a level-2 table. Returns NULL if running out of
 * kernel heap.
 */
static pte_t *
paging_alloc_pgtab(pde_t *pde, bool boot)
{
    pte_t *pgtab = NULL;
    if (boot)
        pgtab = (pte_t *) _kalloc_temp(sizeof(pte_t) * PTES_PER_PAGE, true);
    else
        pgtab = (pte_t *) salloc_page();
    if (pgtab == NULL)
        return NULL;

    memset(pgtab, 0, sizeof(pte_t) * PTES_PER_PAGE);

    pde->present = 1;
    pde->writable = 1;
    pde->user = 1;      /** Just allow user access on all PDEs. */
    pde->frame = ADDR_PAGE_NUMBER((uint32_t) pgtab);

    return pgtab;
}

/**
 * Walk a 2-level page table for a virtual address to locate its PTE.
 * If `alloc` is true, then when a level-2 table is needed but not
 * allocated yet, will perform the allocation.
 */
pte_t *
paging_walk_pgdir(pde_t *pgdir, uint32_t vaddr, bool alloc, bool boot)
{
    size_t pde_idx = ADDR_PDE_INDEX(vaddr);
    size_t pte_idx = ADDR_PTE_INDEX(vaddr);

    /** If already has the level-2 table, return the correct PTE. */
    if (pgdir[pde_idx].present != 0) {
        pte_t *pgtab = (pte_t *) ENTRY_FRAME_ADDR(pgdir[pde_idx]);
        return &pgtab[pte_idx];
    }

    /**
     * Else, the level-2 table is not allocated yet. Do the allocation if
     * the alloc argument is set, otherwise return a NULL.
     */
    if (!alloc)
        return NULL;

    pte_t *pgtab = paging_alloc_pgtab(&pgdir[pde_idx], boot);
    if (pgtab == NULL) {
        warn("walk_pgdir: cannot alloc pgtab, out of kheap memory?");
        return NULL;
    }

    return &pgtab[pte_idx];
}

/** Dealloc all the kernal heap pages used in a user page directory. */
void
paging_destroy_pgdir(pde_t *pgdir)
{
    for (size_t pde_idx = 0; pde_idx < PDES_PER_PAGE; ++pde_idx) {
        if (pgdir[pde_idx].present == 1) {
            pte_t *pgtab = (pte_t *) ENTRY_FRAME_ADDR(pgdir[pde_idx]);
            sfree_page(pgtab);
        }
    }

    /** Free the level-1 directory as well. */
    sfree_page(pgdir);
}


/**
 * Find a free frame and map a user page (given by a pointer to its PTE)
 * into physical memory. Returns the physical address allocated, or 0 if
 * memory allocation failed.
 */
uint32_t
paging_map_upage(pte_t *pte, bool writable)
{
    if (pte->present == 1) {
        error("map_upage: page re-mapping detected");
        return 0;
    }

    uint32_t frame_num = frame_bitmap_alloc();
    if (frame_num == NUM_FRAMES)
        return 0;

    pte->present = 1;
    pte->writable = writable ? 1 : 0;
    pte->user = 1;
    pte->frame = frame_num;

    return ENTRY_FRAME_ADDR((*pte));
}

/** Map a lower-half kernel page to the user PTE. */
void
paging_map_kpage(pte_t *pte, uint32_t paddr)
{
    if (pte->present == 1) {
        error("map_kpage: page re-mapping detected");
        return;
    }

    uint32_t frame_num = ADDR_PAGE_NUMBER(paddr);

    pte->present = 1;
    pte->writable = 0;
    pte->user = 0;      /** User cannot access kernel-mapped pages. */
    pte->frame = frame_num;
}

/**
 * Unmap all the mapped pages within a virtual address range in a user
 * page directory. Avoids calling `wake_pgdir()` repeatedly.
 */
void
paging_unmap_range(pde_t *pgdir, uint32_t va_start, uint32_t va_end)
{
    size_t pde_idx = ADDR_PDE_INDEX(va_start);
    size_t pte_idx = ADDR_PTE_INDEX(va_start);

    size_t pde_end = ADDR_PDE_INDEX(ADDR_PAGE_ROUND_UP(va_end));
    size_t pte_end = ADDR_PTE_INDEX(ADDR_PAGE_ROUND_UP(va_end));

    pte_t *pgtab = (pte_t *) ENTRY_FRAME_ADDR(pgdir[pde_idx]);

    while (pde_idx < pde_end
           || (pde_idx == pde_end && pte_idx < pte_end)) {
        /**
         * If end of current level-2 table, or current level-2 table not
         * allocated, go to the next PDE.
         */
        if (pte_idx >= PTES_PER_PAGE || pgdir[pde_idx].present == 0) {
            pde_idx++;
            pte_idx = 0;
            pgtab = (pte_t *) ENTRY_FRAME_ADDR(pgdir[pde_idx]);
            continue;
        }

        if (pgtab[pte_idx].present == 1) {
            frame_bitmap_clear(pgtab[pte_idx].frame);
            pgtab[pte_idx].present = 0;
            pgtab[pte_idx].writable = 0;
            pgtab[pte_idx].frame = 0;
        }

        pte_idx++;
    }
}

/**
 * Copy all the mapped page within a virtual address range from a page
 * directory to another process's page directory, allocating frames for
 * the new process on the way. Returns false if memory allocation failed.
 */
bool
paging_copy_range(pde_t *dstdir, pde_t *srcdir, uint32_t va_start, uint32_t va_end)
{
    size_t pde_idx = ADDR_PDE_INDEX(va_start);
    size_t pte_idx = ADDR_PTE_INDEX(va_start);

    size_t pde_end = ADDR_PDE_INDEX(ADDR_PAGE_ROUND_UP(va_end));
    size_t pte_end = ADDR_PTE_INDEX(ADDR_PAGE_ROUND_UP(va_end));

    pte_t *srctab = (pte_t *) ENTRY_FRAME_ADDR(srcdir[pde_idx]);
    pte_t *dsttab;

    while (pde_idx < pde_end
           || (pde_idx == pde_end && pte_idx < pte_end)) {
        /**
         * If end of current level-2 table, or current level-2 table not
         * allocated, go to the next PDE.
         */
        if (pte_idx >= PTES_PER_PAGE || srcdir[pde_idx].present == 0) {
            pde_idx++;
            pte_idx = 0;
            srctab = (pte_t *) ENTRY_FRAME_ADDR(srcdir[pde_idx]);
            continue;
        }

        /**
         * If new page directory does not have this level-2 table yet,
         * allocate one for it on kernel heap.
         */
        if (dstdir[pde_idx].present == 0) {
            dsttab = paging_alloc_pgtab(&dstdir[pde_idx], false);
            if (dsttab == NULL) {
                warn("copy_range: cannot alloc pgtab, out of kheap memory?");
                return false;
            }
        }
        dsttab = (pte_t *) ENTRY_FRAME_ADDR(dstdir[pde_idx]);

        /** Map destination frame, and copy source frame content. */
        if (srctab[pte_idx].present == 1) {
            uint32_t paddr = paging_map_upage(&dsttab[pte_idx],
                                              srctab[pte_idx].writable);
            if (paddr == 0) {
                warn("copy_range: cannot map page, out of physical memory?");
                return false;
            }

            memcpy((char *) paddr, (char *) ENTRY_FRAME_ADDR(srctab[pte_idx]),
                   PAGE_SIZE);
        }

        pte_idx++;
    }

    return true;
}


/** Switch the current page directory to the given one. */
inline void
paging_switch_pgdir(pde_t *pgdir)
{
    assert(pgdir != NULL);
    asm volatile ( "movl %0, %%cr3" : : "r" (pgdir) );
}


/** Page fault (ISR # 14) handler. */
static void
page_fault_handler(interrupt_state_t *state)
{
    /** The CR2 register holds the faulty address. */
    uint32_t vaddr;
    asm ( "movl %%cr2, %0" : "=r" (vaddr) : );

    /**
     * Analyze the least significant 3 bits of error code to see what
     * triggered this page fault:
     *   - bit 0: page present -> 1, otherwise 0
     *   - bit 1: is a write operation -> 1, read -> 0
     *   - bit 2: is from user mode -> 1, kernel -> 0
     *
     * See https://wiki.osdev.org/Paging for more.
     */
    bool present = state->err_code & 0x1;
    bool write   = state->err_code & 0x2;
    bool user    = state->err_code & 0x4;

    /** Just prints an information message for now. */
    info("Caught page fault {\n"
         "  faulty addr = %p\n"
         "  present: %d\n"
         "  write:   %d\n"
         "  user:    %d\n"
         "}", vaddr, present, write, user);

    panic("page fault not handled!");
}


/** Initialize paging and switch to use paging. */
void
paging_init(void)
{
    /** Kernel heap starts above all ELF sections. */
    kheap_curr = ADDR_PAGE_ROUND_UP((uint32_t) elf_shstrtab_end);

    /**
     * The frame bitmap also needs space, so allocate space for it in
     * our kernel heap. Clear it to zeros.
     */
    frame_bitmap = (uint32_t *) _kalloc_temp(NUM_FRAMES / 32, false);
    memset(frame_bitmap, 0, NUM_FRAMES / 32);

    /**
     * Allocate the one-page space for the kernel's page directory in
     * the kernel heap. All pages of page directory/tables must be
     * page-aligned.
     */
    kernel_pgdir = (pde_t *) _kalloc_temp(sizeof(pde_t) * PDES_PER_PAGE, true);
    memset(kernel_pgdir, 0, sizeof(pde_t) * PDES_PER_PAGE);

    /**
     * Identity-map the kernel's virtual address space to the physical
     * memory. This means we need to map all the allowed kernel physical
     * frames (from 0 -> KMEM_MAX) as its identity virtual address in
     * the kernel page table, and reserve this entire physical memory region.
     *
     * Assuems that `frame_bitmap_alloc()` behaves sequentially.
     */
    uint32_t addr = 0;
    while (addr < KMEM_MAX) {
        uint32_t frame_num = frame_bitmap_alloc();
        assert(frame_num < NUM_FRAMES);
        pte_t *pte = paging_walk_pgdir(kernel_pgdir, addr, true, true);
        assert(pte != NULL);

        /** Update the bits in this PTE. */
        pte->present = 1;
        pte->writable = 0;      /** Has no affect. */
        pte->user = 0;
        pte->frame = frame_num;

        addr += PAGE_SIZE;
    }

    /**
     * Also map the rest of physical memory into the scheduler page table,
     * so it could access any physical address directly.
     */
    while (addr < PHYS_MAX) {
        pte_t *pte = paging_walk_pgdir(kernel_pgdir, addr, true, true);
        assert(pte != NULL);

        /** Update the bits in this PTE. */
        pte->present = 1;
        pte->writable = 0;      /** Has no affect. */
        pte->user = 0;
        pte->frame = ADDR_PAGE_NUMBER(addr);

        addr += PAGE_SIZE;
    }

    /**
     * Register the page fault handler. This acation must be done before
     * we do the acatual switch towards using paging.
     */
    isr_register(INT_NO_PAGE_FAULT, &page_fault_handler);

    /** Load the address of kernel page directory into CR3. */
    paging_switch_pgdir(kernel_pgdir);

    /**
     * Enable paging by setting the two proper bits of CR0:
     *   - PG bit (31): enable paging
     *   - PE bit (0): enable protected mode
     *   
     * We are not setting the WP bit, so the read/write bit of any PTE just
     * controls whether the page is user writable - in kernel priviledge any
     * page can be written.
     */
    uint32_t cr0;
    asm volatile ( "movl %%cr0, %0" : "=r" (cr0) : );
    cr0 |= 0x80000001;
    asm volatile ( "movl %0, %%cr0" : : "r" (cr0) );
}
