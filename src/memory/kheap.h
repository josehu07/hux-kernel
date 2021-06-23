/**
 * Kernel Heap Memory "Next-Fit" Allocator.
 */


#ifndef KHEAP_H
#define KHEAP_H


#include <stdint.h>
#include <stdbool.h>


/** Random magic number to protect against memory overruns. */
#define KHEAP_MAGIC 0xFBCA0739


/**
 * The free list is embedded inside the heap. Every allocated object (i.e.,
 * memory chunk returned to caller of `kalloc()`) is prefixed with a free-list
 * header structure.
 *
 * See https://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf, figure 17.3
 * ~ figure 17.7. The only different is that our `magic` field is separate
 * from the `next` field and the magic number has weaker protection against
 * buffer overruns.
 */
struct free_list_node_header
{
    size_t size;
    bool free;
    struct free_list_node_header *next;
    uint32_t magic;
};
typedef struct free_list_node_header fl_header_t;


/** Pointer arithmetics helper macros. */
#define HEADER_TO_OBJECT(header) (header + sizeof(fl_header_t))
#define OBJECT_TO_HEADER(object) (object - sizeof(fl_header_t))


void kheap_init();

uint32_t kalloc(size_t size);
void kfree(void *addr);


#endif
