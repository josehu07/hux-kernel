/**
 * User Heap Memory "Next-Fit" Allocator.
 */


#ifndef MALLOC_H
#define MALLOC_H


#include <stdint.h>
#include <stdbool.h>


/** Hardcoded export of the page size and some helper macros. */
#define PAGE_SIZE 4096

#define USER_BASE 0x20000000
#define HEAP_BASE (USER_BASE + 0x00100000)

#define ADDR_PAGE_OFFSET(addr) ((addr) & 0x00000FFF)
#define ADDR_PAGE_NUMBER(addr) ((addr) >> 12)

#define ADDR_PAGE_ALIGNED(addr) (ADDR_PAGE_OFFSET(addr) == 0)

#define ADDR_PAGE_ROUND_DN(addr) ((addr) & 0xFFFFF000)
#define ADDR_PAGE_ROUND_UP(addr) (ADDR_PAGE_ROUND_DN((addr) + 0x00000FFF))


/** Random magic number to protect against memory overruns. */
#define UHEAP_MAGIC 0xEDAF8461


/**
 * The free list is embedded inside the heap. Every allocated object (i.e.,
 * memory chunk returned to caller of `kalloc()`) is prefixed with a free-list
 * header structure.
 *
 * See https://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf, figure 17.3
 * ~ figure 17.7. The only difference is that our `magic` field is separate
 * from the `next` field and the magic number has weaker protection against
 * buffer overruns.
 */
struct free_list_node_header {
    size_t size;
    bool free;
    struct free_list_node_header *next;
    uint32_t magic;
};
typedef struct free_list_node_header fl_header_t;


/** Pointer arithmetics helper macros. */
#define HEADER_TO_OBJECT(header) ((header) + sizeof(fl_header_t))
#define OBJECT_TO_HEADER(object) ((object) - sizeof(fl_header_t))


uint32_t malloc(size_t size);
void mfree(void *addr);


#endif
