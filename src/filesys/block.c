/**
 * Block-level I/O general request layer.
 */


#include <stdint.h>
#include <stdbool.h>

#include "block.h"
#include "vsfs.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../device/idedisk.h"


/**
 * Helper function for reading blocks of data from disk into memory.
 * Uses an internal request buffer, so not zero-copy I/O. DST is the
 * destination buffer, and DISK_ADDR and LEN are both in bytes.
 */
static bool
_block_read(char *dst, uint32_t disk_addr, uint32_t len, bool boot)
{
    block_request_t req;

    uint32_t bytes_read = 0;
    while (len > bytes_read) {
        uint32_t bytes_left = len - bytes_read;

        uint32_t start_addr = disk_addr + bytes_read;
        uint32_t block_no = ADDR_BLOCK_NUMBER(start_addr);
        uint32_t req_offset = ADDR_BLOCK_OFFSET(start_addr);

        uint32_t next_addr = ADDR_BLOCK_ROUND_DN(start_addr) + BLOCK_SIZE;
        uint32_t effective = next_addr - start_addr;
        if (bytes_left < effective)
            effective = bytes_left;

        req.valid = false;
        req.dirty = false;
        req.block_no = block_no;
        bool success = boot ? idedisk_do_req_at_boot(&req)
                            : idedisk_do_req(&req);
        if (!success) {
            warn("block_read: reading IDE disk block %u failed", block_no);
            return false;
        }
        memcpy(dst + bytes_read, req.data + req_offset, effective);

        bytes_read += effective;
    }

    return true;
}

bool
block_read(char *dst, uint32_t disk_addr, uint32_t len)
{
    return _block_read(dst, disk_addr, len, false);
}

bool
block_read_at_boot(char *dst, uint32_t disk_addr, uint32_t len)
{
    return _block_read(dst, disk_addr, len, true);
}

/**
 * Helper function for writing blocks of data from memory into disk.
 * Uses an internal request buffer, so not zero-copy I/O. SRC is the
 * source buffer, and DISK_ADDR and LEN are both in bytes.
 */
bool
block_write(char *src, uint32_t disk_addr, uint32_t len)
{
    block_request_t req;

    uint32_t bytes_written = 0;
    while (len > bytes_written) {
        uint32_t bytes_left = len - bytes_written;

        uint32_t start_addr = disk_addr + bytes_written;
        uint32_t block_no = ADDR_BLOCK_NUMBER(start_addr);
        uint32_t req_offset = ADDR_BLOCK_OFFSET(start_addr);

        uint32_t next_addr = ADDR_BLOCK_ROUND_DN(start_addr) + BLOCK_SIZE;
        uint32_t effective = next_addr - start_addr;
        if (bytes_left < effective)
            effective = bytes_left;

        /**
         * If writing less than a block, first read out the block to
         * avoid corrupting what's already on disk.
         */
        if (effective < BLOCK_SIZE) {
            if (!block_read((char *) req.data, ADDR_BLOCK_ROUND_DN(start_addr),
                            BLOCK_SIZE)) {
                warn("block_write: failed to read out old block %u", block_no);
                return false;
            }
        }

        memcpy(req.data + req_offset, src + bytes_written, effective);
        req.valid = true;
        req.dirty = true;
        req.block_no = block_no;
        if (!idedisk_do_req(&req)) {
            warn("block_write: writing IDE disk block %u failed", block_no);
            return false;
        }

        bytes_written += effective;
    }

    return true;
}


/**
 * Allocate a free data block and mark it in use. Returns the block
 * disk address allocated, or 0 (which is invalid for a data block)
 * on failures.
 */
uint32_t
block_alloc(void)
{
    /** Get a free block from data bitmap. */
    uint32_t slot = bitmap_alloc(&data_bitmap);
    if (slot == data_bitmap.slots) {
        warn("block_alloc: no free data block left");
        return 0;
    }

    if (!data_bitmap_update(slot)) {
        warn("block_alloc: failed to persist data bitmap");
        return 0;
    }

    uint32_t disk_addr = DISK_ADDR_DATA_BLOCK(slot);

    /** Zero the block out for safety. */
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    if (!block_write(buf, disk_addr, BLOCK_SIZE)) {
        warn("block_alloc: failed to zero out block %p", disk_addr);
        bitmap_clear(&data_bitmap, slot);
        data_bitmap_update(slot);   /** Ignores error. */
        return 0;
    }

    return disk_addr;
}

/** Free a disk data block. */
void
block_free(uint32_t disk_addr)
{
    assert(disk_addr >= DISK_ADDR_DATA_BLOCK(0));
    uint32_t slot = (disk_addr / BLOCK_SIZE) - superblock.data_start;

    bitmap_clear(&data_bitmap, slot);
    data_bitmap_update(slot);   /** Ignores error. */

    /** Zero the block out for safety. */
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    if (!block_write(buf, ADDR_BLOCK_ROUND_DN(disk_addr), BLOCK_SIZE))
        warn("block_free: failed to zero out block %p", disk_addr);
}
