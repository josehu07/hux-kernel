/**
 * User Heap Memory "Next-Fit" Allocator.
 */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "malloc.h"
#include "syscall.h"
#include "string.h"
#include "debug.h"
#include "printf.h"


static uint32_t uheap_btm;
static uint32_t uheap_top;
static bool uheap_initialized = false;


/** Global state of the free-list. */
static fl_header_t *bottom_most_header;     /** Address-wise smallest node. */
static fl_header_t *last_search_header;     /** Where the last search ends. */
static size_t free_list_length = 0;


/** For debug printing the state of the free-list. */
__attribute__((unused))
static void
_print_free_list_state(void)
{
    fl_header_t *header_curr = bottom_most_header;

    info("Uheap free-list length = %d, last_search = %p, nodes:",\
         free_list_length, last_search_header);
    do {
        printf("  node header %p { size: %d, next: %p }\n",
               header_curr, header_curr->size, header_curr->next);

        header_curr = (fl_header_t *) header_curr->next;
    } while (header_curr != bottom_most_header);
}


/** Initialize the heap by setting up the first chunk. */
static bool
_heap_init(void)
{
    assert(!uheap_initialized);

    /** Make the first heap page. */
    uheap_btm = HEAP_BASE;
    uheap_top = uheap_btm + PAGE_SIZE;
    if (setheap(uheap_top) != 0) {
        warn("malloc: cannot initialize heap, out of memory?");
        return false;
    }

    fl_header_t *header = (fl_header_t *) uheap_btm;
    uint32_t size = (uheap_top - uheap_btm) - sizeof(fl_header_t);
    memset((char *) (HEADER_TO_OBJECT(uheap_btm)), 0, size);

    header->size = size;
    header->free = true;
    header->next = header;      /** Point to self initially. */
    header->magic = UHEAP_MAGIC;

    bottom_most_header = header;
    last_search_header = header;
    free_list_length = 1;

    uheap_initialized = true;
    return true;
}

/** Extend the heap upper boundary to hold at least size bytes. */
static bool
_heap_enlarge(size_t size)
{
    uint32_t new_top = ADDR_PAGE_ROUND_UP(uheap_top + size);
    if (setheap(new_top) != 0) {
        warn("malloc: cannot extend heap boundary, out of memory?");
        return false;
    }

    /**
     * Make the new pages a new free chunk, and merge it into the old last
     * chunk if that touches the old upper boundary.
     */
    fl_header_t *header = (fl_header_t *) uheap_top;
    uint32_t chunk_size = (new_top - uheap_top) - sizeof(fl_header_t);
    memset((char *) (HEADER_TO_OBJECT(uheap_top)), 0, chunk_size);

    header->size = chunk_size;
    header->free = true;
    header->magic = UHEAP_MAGIC;

    if (free_list_length == 0) {
        header->next = header;
        bottom_most_header = header;
        last_search_header = header;
        free_list_length++;
    } else {
        fl_header_t *dn_header = bottom_most_header;
        while (dn_header->next != bottom_most_header)
            dn_header = dn_header->next;
        bool dn_coalesable =
            HEADER_TO_OBJECT((uint32_t) dn_header) + dn_header->size == (uint32_t) header;

        if (dn_coalesable)
            dn_header->size += header->size + sizeof(fl_header_t);
        else {
            dn_header->next = header;
            header->next = bottom_most_header;
            free_list_length++;
        }
    }

    return true;
}


/**
 * Allocate a piece of heap memory (an object) of given size. Adopts
 * the "next-fit" allocation policy. When there is no free region in
 * currently allocated heap pages to serve a request, calls the `setheap()`
 * syscall to get more heap pages. Returns 0 on failure.
 */
uint32_t
malloc(size_t size)
{
    if (!uheap_initialized) {
        if (!_heap_init())
            return 0;
    }

    /** It could be previous allocations used up exactly the entire heap. */
    if (free_list_length == 0) {
        if (!_heap_enlarge(size))
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
            header_new->magic = UHEAP_MAGIC;
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

        // _print_free_list_state();
        return object;

    } while (header_curr != header_begin);

    /** No free chunk is large enough, enlarge and retry. */
    if (!_heap_enlarge(size))
        return 0;
    return malloc(size);
}


/**
 * Free a previously allocated object address, and merges it into the
 * free-list if any neighboring chunk is free at the time.
 */
void
mfree(void *addr)
{
    /** Cannot free if no `malloc()` has been called. */
    assert(uheap_initialized);

    fl_header_t *header = (fl_header_t *) OBJECT_TO_HEADER((uint32_t) addr);

    if ((uint32_t) addr < uheap_btm || (uint32_t) addr >= uheap_top) {
        error("mfree: object %p is out of user heap range", addr);
        return;
    }

    if (header->magic != UHEAP_MAGIC) {
        error("mfree: object %p corrupted its header magic", addr);
        return;
    }

    /** Fill with zero bytes to catch dangling pointers use. */
    header->free = true;
    memset((char *) addr, 0, header->size);

    /**
     * Special case of empty free-list (all bytes exactly allocated before
     * this `mfree()` call). If so, just add this free'd obejct as a node.
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

    // _print_free_list_state();
}
