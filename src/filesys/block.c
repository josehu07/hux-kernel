/**
 * Block-level I/O general request layer.
 */


#include <stdint.h>
#include <stdbool.h>

#include "block.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../device/idedisk.h"


/**
 * Helper function for reading blocks of data from disk into memory.
 * Uses an internal request buffer, so not zero-copy I/O. DST is the
 * destination buffer, and DISK_ADDR and LEN are both in bytes.
 */
bool
block_read(char *dst, uint32_t disk_addr, uint32_t len, bool boot)
{
    block_request_t req;

    uint32_t bytes_read = 0;
    while (len > bytes_read) {
        uint32_t bytes_left = len - bytes_read;

        uint32_t start_addr = disk_addr + bytes_read;
        uint32_t block_no = ADDR_BLOCK_NUMBER(start_addr);

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
            warn("disk_read: reading IDE disk block %u failed", block_no);
            return false;
        }

        memcpy(dst + bytes_read, req.data, effective);
        bytes_read += effective;
    }

    return true;
}
