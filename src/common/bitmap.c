/**
 * Bitmap data structure used in paging, file system, etc.
 */


#include <stdint.h>
#include <stdbool.h>

#include "bitmap.h"
#include "debug.h"
#include "string.h"
#include "spinlock.h"


/** Set a slot as used. */
inline void
bitmap_set(bitmap_t *bitmap, uint32_t slot_no)
{
    assert(slot_no < bitmap->slots);

    bool was_locked = spinlock_locked(&(bitmap->lock));
    if (!was_locked)
        spinlock_acquire(&(bitmap->lock));

    size_t outer_idx = BITMAP_OUTER_IDX(slot_no);
    size_t inner_idx = BITMAP_INNER_IDX(slot_no);
    bitmap->bits[outer_idx] |= (1 << (7 - inner_idx));

    if (!was_locked)
        spinlock_release(&(bitmap->lock));
}

/** Clear a slot as free. */
inline void
bitmap_clear(bitmap_t *bitmap, uint32_t slot_no)
{
    assert(slot_no < bitmap->slots);
    if (slot_no == 0)
        info("clearing 0");

    spinlock_acquire(&(bitmap->lock));

    size_t outer_idx = BITMAP_OUTER_IDX(slot_no);
    size_t inner_idx = BITMAP_INNER_IDX(slot_no);
    bitmap->bits[outer_idx] &= ~(1 << (7 - inner_idx));

    spinlock_release(&(bitmap->lock));
}

/** Returns true if a slot is in use, otherwise false. */
inline bool
bitmap_check(bitmap_t *bitmap, uint32_t slot_no)
{
    assert(slot_no < bitmap->slots);
    
    spinlock_acquire(&(bitmap->lock));

    size_t outer_idx = BITMAP_OUTER_IDX(slot_no);
    size_t inner_idx = BITMAP_INNER_IDX(slot_no);
    bool result = bitmap->bits[outer_idx] & (1 << (7 - inner_idx));

    spinlock_release(&(bitmap->lock));

    return result;
}

/**
 * Allocate a slot and mark as used. Returns the slot number of
 * the allocated slot, or `num_slots` if there is no free slot.
 */
uint32_t
bitmap_alloc(bitmap_t *bitmap)
{
    spinlock_acquire(&(bitmap->lock));

    for (size_t i = 0; i < (bitmap->slots / 8); ++i) {
        if (bitmap->bits[i] == 0xFF)
            continue;
        for (size_t j = 0; j < 8; ++j) {
            if ((bitmap->bits[i] & (1 << (7 - j))) == 0) {
                /** Found a free slot. */
                uint32_t slot_no = i * 8 + j;
                bitmap_set(bitmap, slot_no);

                spinlock_release(&(bitmap->lock));
                return slot_no;
            }
        }
    }

    spinlock_release(&(bitmap->lock));
    return bitmap->slots;
}


/** Initialize the bitmap. BITS must have been allocated. */
void
bitmap_init(bitmap_t *bitmap, uint8_t *bits, uint32_t slots)
{
    bitmap->slots = slots;
    bitmap->bits = bits;
    memset(bits, 0, slots / 8);
    spinlock_init(&(bitmap->lock), "bitmap's spinlock");
}
