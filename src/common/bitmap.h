/**
 * Bitmap data structure used in paging, file system, etc.
 */


#ifndef BITMAP_H
#define BITMAP_H


#include <stdint.h>
#include <stdbool.h>


/** Bitmap is simply a contiguous array of bits. */
struct bitmap {
    uint32_t *bits;     /** Must ensure zero'ed out at intialization! */
    uint32_t slots;     /** Must be a multiple of 32, and set at initialization. */
};
typedef struct bitmap bitmap_t;


/**
 * Every bit indicates the free/used state of a corresponding slot
 * of something. Slot number one-one maps to bit index.
 */
#define BITMAP_OUTER_IDX(slot_num) ((slot_num) / 32)
#define BITMAP_INNER_IDX(slot_num) ((slot_num) % 32)


void bitmap_set(bitmap_t *bitmap, uint32_t slot_no);
void bitmap_clear(bitmap_t *bitmap, uint32_t slot_no);
bool bitmap_check(bitmap_t *bitmap, uint32_t slot_no);
uint32_t bitmap_alloc(bitmap_t *bitmap);


#endif