/**
 * Kernel Heap Memory "Next-Fit" Allocator.
 */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "kheap.h"
#include "paging.h"

#include "../common/printf.h"
#include "../common/string.h"
#include "../common/debug.h"
#include "../common/spinlock.h"


static uint32_t kheap_btm;
static uint32_t kheap_top;


/** Global state of the free-list. */
static fl_header_t *bottom_most_header;     /** Address-wise smallest node. */
static fl_header_t *last_search_header;     /** Where the last search ends. */
static size_t free_list_length = 1;

static spinlock_t kheap_lock;


/** For debug printing the state of the free-list. */
__attribute__((unused))
static void
_print_free_list_state(void)
{
    spinlock_acquire(&kheap_lock);

    fl_header_t *header_curr = bottom_most_header;

    info("Kheap free-list length = %d, last_search = %p, nodes:",\
         free_list_length, last_search_header);
    do {
        printf("  node header %p { size: %d, next: %p }\n",
               header_curr, header_curr->size, header_curr->next);

        header_curr = (fl_header_t *) header_curr->next;
    } while (header_curr != bottom_most_header);

    spinlock_release(&kheap_lock);
}


/**
 * Allocate a piece of heap memory (an object) of given size. Adopts
 * the "next-fit" allocation policy. Returns 0 on failure.
 */
uint32_t
kalloc(size_t size)
{
    spinlock_acquire(&kheap_lock);

    if (free_list_length == 0) {
        warn("kalloc: kernel flexible heap all used up");
        spinlock_release(&kheap_lock);
        return 0;
    }

    fl_header_t *header_last = last_search_header;
    fl_header_t *header_curr = (fl_header_t *) header_last->next;
    fl_header_t *header_begin = header_curr;

    do {
        /** If this node is not large enough, skip. */
        if (header_curr->size < size) {
            header_last = header_curr;
            header_curr = (fl_header_t *) header_curr->next;
            continue;
        }

        /**
         * Else, split this node if it is larger than `size` + meta bytes
         * (after split, the rest of it can be made a smaller free chunk).
         */
        if (header_curr->size > size + sizeof(fl_header_t)) {
            /** Split out a new node. */
            fl_header_t *header_new = (fl_header_t *)
                ((uint32_t) header_curr + size + sizeof(fl_header_t));
            header_new->size = (header_curr->size - size) - sizeof(fl_header_t);
            header_new->magic = KHEAP_MAGIC;
            header_new->free = true;

            /** Don't forget to update the current node's size. */
            header_curr->size = size;

            /**
             * Now, update the links between nodes. The special case of list
             * only having one node needs to be taken care of.
             * 
             * If only one node in list, then `header_last` == `header_curr`,
             * so `last_search_header` should be the new node, not the current
             * node (that is about to be returned as an object).
             */
            if (free_list_length == 1) {
                header_new->next = header_new;
                last_search_header = header_new;
            } else {
                header_new->next = header_curr->next;
                header_last->next = header_new;
                last_search_header = header_last;
            }

            /** Update smallest-address node. */
            if (header_curr == bottom_most_header)
                bottom_most_header = header_new;
        
        } else {
            /** Not splitting, then just take this node off the list. */
            header_last->next = header_curr->next;
            free_list_length--;

            /** Update smallest-address node. */
            if (header_curr == bottom_most_header)
                bottom_most_header = header_curr->next;
        }

        /** `header_curr` is now the chunk to return. */
        header_curr->next = NULL;
        header_curr->free = false;
        uint32_t object = HEADER_TO_OBJECT((uint32_t) header_curr);

        spinlock_release(&kheap_lock);
        
        // _print_free_list_state();
        return object;

    } while (header_curr != header_begin);

    /** No free chunk is large enough, time to panic. */
    warn("kalloc: no free chunk large enough for size %d\n", size);
    spinlock_release(&kheap_lock);
    return 0;
}


/**
 * Free a previously allocated object address, and merges it into the
 * free-list if any neighboring chunk is free at the time.
 */
void
kfree(void *addr)
{
    fl_header_t *header = (fl_header_t *) OBJECT_TO_HEADER((uint32_t) addr);

    if ((uint32_t) addr < kheap_btm || (uint32_t) addr >= kheap_top) {
        error("kfree: object %p is out of heap range", addr);
        return;
    }

    if (header->magic != KHEAP_MAGIC) {
        error("kfree: object %p corrupted its header magic", addr);
        return;
    }

    /** Fill with zero bytes to catch dangling pointers use. */
    header->free = true;
    memset((char *) addr, 0, header->size);

    spinlock_acquire(&kheap_lock);

    /**
     * Special case of empty free-list (all bytes exactly allocated before
     * this `kfree()` call). If so, just add this free'd obejct as a node.
     */
    if (free_list_length == 0) {
        header->next = header;
        bottom_most_header = header;
        last_search_header = header;
        free_list_length++;

    /**
     * Else, traverse the free-list starting form the bottom-most node to
     * find the proper place to insert this free'd node, and then check if
     * any of its up- & down-neighbor is free. If so, coalesce with them
     * into a bigger free chunk.
     *
     * This linked-list traversal is not quite efficient and there are tons
     * of ways to optimize the free-list structure, e.g., using a balanced
     * binary search tree. Left as future work.
     */
    } else {
        /**
         * Locate down-neighbor. Will get the top-most node if the free'd
         * object is located below the current bottom-most node.
         */
        fl_header_t *dn_header = bottom_most_header;
        while (dn_header->next != bottom_most_header) {
            if (dn_header < header && dn_header->next > header)
                break;
            dn_header = dn_header->next;
        }
        bool dn_coalesable = dn_header > header ? false
            : HEADER_TO_OBJECT((uint32_t) dn_header) + dn_header->size == (uint32_t) header;

        /**
         * Locate up-neighbor. Will get the bottom-most node if the free'd
         * object is located above all the free nodes.
         */
        fl_header_t *up_header = dn_header > header ? bottom_most_header
            : dn_header->next;
        bool up_coalesable = up_header < header ? false
            : HEADER_TO_OBJECT((uint32_t) header) + header->size == (uint32_t) up_header;

        /** Case #1: Both coalesable. */
        if (dn_coalesable && up_coalesable) {
            /** Remove up-neighbor from list. */
            dn_header->next = up_header->next;
            free_list_length--;
            /** Extend down-neighbor to cover the whole region. */
            dn_header->size +=
                header->size + up_header->size + 2 * sizeof(fl_header_t);
            /** Update last search node. */
            if (last_search_header == up_header)
                last_search_header = dn_header;

        /** Case #2: Only with down-neighbor. */
        } else if (dn_coalesable) {
            /** Extend down-neighbor to cover me. */
            dn_header->size += header->size + sizeof(fl_header_t);

        /** Case #3: Only with up neighbor. */
        } else if (up_coalesable) {
            /** Update dn-neighbor to point to me. */
            dn_header->next = header;
            /** Extend myself to cover up-neighbor. */
            header->size += up_header->size + sizeof(fl_header_t);
            header->next = up_header->next;
            /** Update bottom-most node. */
            if (bottom_most_header > header)
                bottom_most_header = header;
            /** Update last search node. */
            if (last_search_header == up_header)
                last_search_header = header;

        /** Case #4: With neither. */
        } else {
            /** Link myself in the list. */
            dn_header->next = header;
            header->next = up_header;
            free_list_length++;
            /** Update bottom-most node. */
            if (bottom_most_header > header)
                bottom_most_header = header;
        }
    }

    spinlock_release(&kheap_lock);
    // _print_free_list_state();
}


/** Initialize the kernel heap allocator. */
void
kheap_init(void)
{
    /**
     * Initially, everything from `kheap_curr` (page rounded-up) upto 8MiB
     * are free heap memory available to use. Initialize it as one big
     * free chunk.
     */
    kheap_btm = kheap_curr;
    kheap_top = KHEAP_MAX;

    fl_header_t *header = (fl_header_t *) kheap_btm;
    uint32_t size = (kheap_top - kheap_btm) - sizeof(fl_header_t);
    memset((char *) (HEADER_TO_OBJECT(kheap_btm)), 0, size);

    header->size = size;
    header->free = true;
    header->next = header;      /** Point to self initially. */
    header->magic = KHEAP_MAGIC;

    bottom_most_header = header;
    last_search_header = header;
    free_list_length = 1;

    spinlock_init(&kheap_lock, "kheap_lock");
}
