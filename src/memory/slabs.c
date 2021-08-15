/**
 * Simple SLAB allocators for fixed-granularity kernel objects.
 */


#include <stdint.h>
#include <stddef.h>

#include "slabs.h"
#include "paging.h"

#include "../common/debug.h"
#include "../common/string.h"
#include "../common/spinlock.h"


/** Page-granularity SLAB free-list. */
static uint32_t page_slab_btm;
static uint32_t page_slab_top;

static slab_node_t *page_slab_freelist;
static spinlock_t page_slab_lock;


/**
 * Internal generic SLAB allocator. Give it a pointer to the pointer to
 * the first node of any initialized fixed-granularity free-list.
 *
 * There is no data integrity checks on magics. Returns 0 on failures.
 * Must be called with the corresponding SLAB's lock held.
 */
static uint32_t
_salloc_internal(slab_node_t **freelist)
{
    if (freelist == NULL) {
        warn("salloc: given free-list pointer is NULL");
        return 0;
    }

    slab_node_t *node = *freelist;

    /** No slab is free, time to panic. */
    if (node == NULL) {
        warn("salloc: free-list %p has no free slabs", freelist);
        return 0;
    }

    *freelist = node->next;
    
    return (uint32_t) node;
}

/** Wrappers for differnet granularities. */
uint32_t
salloc_page(void)
{
    spinlock_acquire(&page_slab_lock);
    uint32_t addr = _salloc_internal(&page_slab_freelist);
    spinlock_release(&page_slab_lock);

    return addr;
}


/**
 * Internal generic SLAB deallocator. Assumes the address is valid and
 * properly-aligned to the granularity.
 *
 * Must be called with the corresponding SLAB's lock held.
 */
static void
_sfree_internal(slab_node_t **freelist, void *addr)
{
    slab_node_t *node = (slab_node_t *) addr;

    /** Simply insert to the head of free-list. */
    node->next = *freelist;
    *freelist = node;
}

/** Wrapper for different granularities. */
void
sfree_page(void *addr)
{
    if ((uint32_t) addr < page_slab_btm || (uint32_t) addr >= page_slab_top) {
        warn("sfree_page: object %p is out of page slab range", addr);
        return;
    }

    if ((uint32_t) addr % PAGE_SIZE != 0) {
        warn("sfree_page: object %p is not page-aligned", addr);
        return;
    }

    /** Fill with zero bytes to catch dangling pointers use. */
    memset((char *) addr, 0, PAGE_SIZE);
    
    spinlock_acquire(&page_slab_lock);
    _sfree_internal(&page_slab_freelist, addr);
    spinlock_release(&page_slab_lock);
}


/** Initializers for SLAB allocators. */
void
page_slab_init(void)
{
    page_slab_btm = PAGE_SLAB_MIN;
    page_slab_top = PAGE_SLAB_MAX;

    page_slab_freelist = NULL;

    for (uint32_t addr = page_slab_btm;
         addr < page_slab_top;
         addr += PAGE_SIZE) {
        sfree_page((char *) addr);
    }

    spinlock_init(&page_slab_lock, "page_slab_lock");
}
