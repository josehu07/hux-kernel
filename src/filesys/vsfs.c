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
#include "../common/bitmap.h"
#include "../common/spinlock.h"
#include "../common/parklock.h"

#include "../memory/kheap.h"
#include "../memory/slabs.h"


/** In-memory runtime copy of the FS data structures. */
superblock_t superblock;

static bitmap_t inode_bitmap;
static bitmap_t data_bitmap;


/**
 * Initialize the file system by reading out the image from the
 * IDE disk and parse according to VSFS layout.
 */
void
filesys_init(void)
{
    /** Block 0 must be the superblock. */
    if (!block_read((char *) &superblock, 0, sizeof(superblock_t), true))
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
    uint32_t num_inodes = superblock.inode_blocks * (BLOCK_SIZE / INODE_SIZE);
    uint32_t *inode_bits = (uint32_t *) kalloc(num_inodes / 8);
    bitmap_init(&inode_bitmap, inode_bits, num_inodes);
    if (!block_read((char *) inode_bitmap.bits,
                    superblock.inode_bitmap_start * BLOCK_SIZE,
                    num_inodes / 8, true)) {
        error("filesys_init: failed to read inode bitmap from disk");
    }

    uint32_t num_dblocks = superblock.data_blocks;
    uint32_t *data_bits = (uint32_t *) kalloc(num_dblocks / 8);
    bitmap_init(&data_bitmap, data_bits, num_dblocks);
    if (!block_read((char *) data_bitmap.bits,
                    superblock.data_bitmap_start * BLOCK_SIZE,
                    num_dblocks / 8, true)) {
        error("filesys_init: failed to read data bitmap from disk");
    }

    /** Fill open file table and inode table with empty slots. */
    for (size_t i = 0; i < MAX_OPEN_FILES; ++i) {
        ftable[i].ref_cnt = 0;      /** Indicates UNUSED. */
        ftable[i].readable = false;
        ftable[i].writable = false;
        ftable[i].inode = NULL;
        ftable[i].offset = 0;
    }
    spinlock_init(&ftable_lock, "ftable_lock");

    for (size_t i = 0; i < MAX_MEM_INODES; ++i) {
        icache[i].ref_cnt = 0;      /** Indicates UNUSED. */
        icache[i].inumber = 0;
        parklock_init(&(icache[i].lock), "inode's parklock");
    }
    spinlock_init(&icache_lock, "icache_lock");
}
