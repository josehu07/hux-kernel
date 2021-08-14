/**
 * Very simple file system (VSFS) data structures & Layout.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "vsfs.h"
#include "block.h"
#include "file.h"

#include "../common/debug.h"
#include "../common/string.h"
#include "../common/bitmap.h"

#include "../device/idedisk.h"

#include "../memory/kheap.h"
#include "../memory/slabs.h"


/** In-memory runtime copy of the FS data structures. */
superblock_t superblock;

static bitmap_t inode_bitmap;
static bitmap_t data_bitmap;


/**
 * Helper function for reading blocks of data from disk into memory.
 * Uses an internal request buffer, so not zero-copy I/O. DST is the
 * destination buffer, and DISK_ADDR and LEN are both in bytes.
 */
static bool
_disk_read(char *dst, uint32_t disk_addr, uint32_t len, bool boot)
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


/**
 * Initialize the file system by reading out the image from the
 * IDE disk and parse according to VSFS layout.
 */
void
filesys_init(void)
{
    /** Block 0 must be the superblock. */
    if (!_disk_read((char *) &superblock, 0, sizeof(superblock_t), true))
        error("filesys_init: failed to read superblock from disk");

    /**
     * Currently the VSFS layout is hard-defined, so just do asserts here
     * to ensure that the mkfs script followed the expected layout. In real
     * systems, mkfs should have the flexibility to generate FS image as
     * long as the layout is valid, and we read out the actual layout here.
     */
    assert(superblock.fs_blocks == 262144);
    assert(superblock.inode_bitmap_start == 1);
    assert(superblock.inode_bitmap_blocks == 6);
    assert(superblock.data_bitmap_start == 7);
    assert(superblock.data_bitmap_blocks == 32);
    assert(superblock.inode_start == 39);
    assert(superblock.inode_blocks == 6105);
    assert(superblock.data_start == 6144);
    assert(superblock.data_blocks == 256000);

    /** Read in the two bitmaps into memory. */
    inode_bitmap.slots = superblock.inode_blocks * (BLOCK_SIZE / sizeof(inode_t));
    size_t inode_bitmap_size = (inode_bitmap.slots / 32) * sizeof(uint32_t);
    inode_bitmap.bits = (uint32_t *) kalloc(inode_bitmap_size);
    if (!_disk_read((char *) inode_bitmap.bits,
                    superblock.inode_bitmap_start * BLOCK_SIZE,
                    inode_bitmap_size, true)) {
        error("filesys_init: failed to read inode bitmap from disk");
    }

    data_bitmap.slots = superblock.data_blocks;
    size_t data_bitmap_size = (data_bitmap.slots / 32) * sizeof(uint32_t);
    if (!_disk_read((char *) data_bitmap.bits,
                    superblock.data_bitmap_start * BLOCK_SIZE,
                    data_bitmap_size, true)) {
        error("filesys_init: failed to read data bitmap from disk");
    }

    // TODO: read in root directory

    /** Fill the open file table with empty slots. */
    for (size_t i = 0; i < MAX_FILES; ++i) {
        ftable[i].ref_cnt = 0;      /** Indicates UNUSED. */
        ftable[i].readable = false;
        ftable[i].writable = false;
        ftable[i].inode = NULL;
        ftable[i].offset = 0;
    }
}
