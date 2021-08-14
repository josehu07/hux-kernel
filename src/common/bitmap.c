/**
 * Bitmap data structure used in paging, file system, etc.
 */


#include <stdint.h>
#include <stdbool.h>

#include "bitmap.h"
#include "debug.h"
#include "intstate.h"


/** Set a slot as used. */
inline void
bitmap_set(bitmap_t *bitmap, uint32_t slot_no)
{
    assert(slot_no < bitmap->slots);

    cli_push();

    size_t outer_idx = BITMAP_OUTER_IDX(slot_no);
    size_t inner_idx = BITMAP_INNER_IDX(slot_no);
    bitmap->bits[outer_idx] |= (1 << inner_idx);

    cli_pop();
}

/** Clear a slot as free. */
inline void
bitmap_clear(bitmap_t *bitmap, uint32_t slot_no)
{
    assert(slot_no < bitmap->slots);

    cli_push();

    size_t outer_idx = BITMAP_OUTER_IDX(slot_no);
    size_t inner_idx = BITMAP_INNER_IDX(slot_no);
    bitmap->bits[outer_idx] &= ~(1 << inner_idx);

    cli_pop();
}

/** Returns true if a slot is in use, otherwise false. */
inline bool
bitmap_check(bitmap_t *bitmap, uint32_t slot_no)
{
    assert(slot_no < bitmap->slots);
    
    cli_push();

    size_t outer_idx = BITMAP_OUTER_IDX(slot_no);
    size_t inner_idx = BITMAP_INNER_IDX(slot_no);
    bool result = bitmap->bits[outer_idx] & (1 << inner_idx);

    cli_pop();

    return result;
}

/**
 * Allocate a slot and mark as used. Returns the slot number of
 * the allocated slot, or `num_slots` if there is no free slot.
 */
uint32_t
bitmap_alloc(bitmap_t *bitmap)
{
    cli_push();

    for (size_t i = 0; i < (bitmap->slots / 32); ++i) {
        if (bitmap->bits[i] == 0xFFFFFFFF)
            continue;
        for (size_t j = 0; j < 32; ++j) {
            if ((bitmap->bits[i] & (1 << j)) == 0) {
                /** Found a free slot. */
                uint32_t slot_no = i * 32 + j;
                bitmap_set(bitmap, slot_no);

                cli_pop();
                return i * 32 + j;
            }
        }
    }

    cli_pop();
    return bitmap->slots;
}
