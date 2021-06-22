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


/** Temporary solution: kernel heap bottom address. */
extern uint32_t bss_end;    /** Defined in linker script. */
static uint32_t kheap_curr = (uint32_t) &bss_end;

/** Bitmap indicating free/used frames. */
static uint32_t *frame_bitmap;

/**
 * Pointer to current active page directory, may be the kernel's page
 * directory or the current running process's page directory.
 */
static pde_t *active_pgdir;
static pde_t *kernel_pgdir;     /** Allocated at paging init. */


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
        error("kernel memory exceeds boundary");

    uint32_t temp = kheap_curr;
    kheap_curr += size;
    return temp;
}


/**
 * Helper functions for managing free physical frames, using a bitmap
 * data structure. Every bit indicates the free/used state of a corresponding
 * physical frame. Frame number one-one maps to bit index.
 */
#define BITMAP_OUTER_IDX(frame_num) (frame_num / 32)
#define BITMAP_INNER_IDX(frame_num) (frame_num % 32)

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

    error("failed to find a free frame");
    return NUM_FRAMES;
}


/**
 * Walk a 2-level page table for a virtual address to locate its PTE.
 * If `alloc` is true, then when a level-2 table is needed but not
 * allocated yet, will perform the allocation.
 */
pte_t *
paging_walk_pgdir(pde_t *pgdir, uint32_t vaddr, bool alloc)
{
    size_t pde_idx = ADDR_PDE_INDEX(vaddr);
    size_t pte_idx = ADDR_PTE_INDEX(vaddr);

    /** If already has the level-2 table, return the correct PTE. */
    if (pgdir[pde_idx].present != 0) {
        pte_t *pgtab = (pte_t *) ENTRY_FRAME_ADDR((uint32_t) pgdir[pde_idx]);
        return &pgtab[pte_idx];
    }

    /**
     * Else, the level-2 table is not allocated yet. Do the allocation if
     * the alloc argument is set, otherwise return a NULL.
     */
    if (!alloc)
        return NULL;

    pte_t *pgtab = (pte_t *) _kalloc_temp(sizeof(pte_t) * PTES_PER_PAGE, true);
    memset(pgtab, 0, sizeof(pte_t) * PTES_PER_PAGE);

    pgdir[pde_idx].present = 1;
    pgdir[pde_idx].writable = 0;
    pgdir[pde_idx].user = 0;
    pgdir[pde_idx].frame = ADDR_PAGE_NUMBER((uint32_t) pgtab);

    return &pgtab[pte_idx];
}


/** Switch the current page directory to the given one. */
void
paging_switch_pgdir(pde_t *pgdir)
{
    active_pgdir = pgdir;
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
         "  faulty addr = %#X\n"
         "  present: %d\n"
         "  write:   %d\n"
         "  user:    %d\n"
         "}", vaddr, present, write, user);

    /**
     * Temporary incomplete handler logic: alloc a frame for the faulty
     * page if it is the kernel trying to read a faulty address which
     * is not present.
     */
    // if ((!present) && (!user)) {
    //     pte_t *pte = paging_walk_pgdir(kernel_pgdir, vaddr, true);

    // }
    panic("page fault not handled!");
}


/** Initialize paging and switch to use paging. */
void
paging_init(void)
{
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
    active_pgdir = kernel_pgdir;

    /**
     * Identity-map the kernel's virtual address space to the physical
     * memory. This means we need to map all the allowed kernel physical
     * frames (from 0 -> KMEM_MAX) as its identity virtual address in
     * the kernel page table.
     */
    uint32_t addr = 0;
    while (addr < KMEM_MAX) {
        uint32_t frame_num = frame_bitmap_alloc();
        pte_t *pte = paging_walk_pgdir(kernel_pgdir, addr, true);

        /** Update the bits in this PTE. */
        pte->present = 1;
        pte->writable = 0;
        pte->user = 0;
        pte->frame = frame_num;

        addr += PAGE_SIZE;
    }

    /**
     * Register the page fault handler. This acation must be done before
     * we do the acatual switch towards using paging.
     */
    isr_register(INT_NO_PAGE_FAULT, page_fault_handler);

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
