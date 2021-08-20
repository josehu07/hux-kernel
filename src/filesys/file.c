/**
 * In-memory structures and operations on open files.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "file.h"
#include "block.h"
#include "vsfs.h"
#include "sysfile.h"

#include "../common/debug.h"
#include "../common/string.h"
#include "../common/spinlock.h"
#include "../common/parklock.h"


/** Open inode cache - list of in-memory inode structures. */
mem_inode_t icache[MAX_MEM_INODES];
spinlock_t icache_lock;

/** Open file table - list of open file structures. */
file_t ftable[MAX_OPEN_FILES];
spinlock_t ftable_lock;


/** For debug printing the state of the two tables. */
__attribute__((unused))
static void
_print_icache_state(void)
{
    spinlock_acquire(&icache_lock);

    info("Inode cache state:");
    for (mem_inode_t *inode = icache; inode < &icache[MAX_MEM_INODES]; ++inode) {
        if (inode->ref_cnt == 0)
            continue;
        printf("  inode { inum: %u, ref_cnt: %d, size: %u, dir: %b }\n",
               inode->inumber, inode->ref_cnt, inode->d_inode.size,
               inode->d_inode.type == INODE_TYPE_DIR ? 1 : 0);
    }
    printf("  end\n");

    spinlock_release(&icache_lock);
}

__attribute__((unused))
static void
_print_ftable_state(void)
{
    spinlock_acquire(&ftable_lock);

    info("Open file table state:");
    for (file_t *file = ftable; file < &ftable[MAX_OPEN_FILES]; ++file) {
        if (file->ref_cnt == 0)
            continue;
        printf("  file -> inum %u { ref_cnt: %d, offset: %u, r: %b, w: %b }\n",
               file->inode->inumber, file->ref_cnt, file->offset,
               file->readable, file->writable);
    }
    printf("  end\n");

    spinlock_release(&ftable_lock);
}


/** Lock/Unlock a mem_ionde's private fields. */
void
inode_lock(mem_inode_t *m_inode)
{
    parklock_acquire(&(m_inode->lock));
}

void
inode_unlock(mem_inode_t *m_inode)
{
    parklock_release(&(m_inode->lock));
}


/**
 * Get inode for given inode number. If that inode has been in memory,
 * increment its ref count and return. Otherwise, read from the disk into
 * an empty inode cache slot.
 */
static mem_inode_t *
_inode_get(uint32_t inumber, bool boot)
{
    assert(inumber < (superblock.inode_blocks * (BLOCK_SIZE / INODE_SIZE)));

    mem_inode_t *m_inode = NULL;

    spinlock_acquire(&icache_lock);

    /** Search icache to see if it has been in memory. */
    mem_inode_t *empty_slot = NULL;
    for (m_inode = icache; m_inode < &icache[MAX_MEM_INODES]; ++m_inode) {
        if (m_inode->ref_cnt > 0 && m_inode->inumber == inumber) {
            m_inode->ref_cnt++;
            spinlock_release(&icache_lock);
            return m_inode;
        }
        if (empty_slot == NULL && m_inode->ref_cnt == 0)
            empty_slot = m_inode;   /** Remember empty slot seen. */
    }

    if (empty_slot == NULL) {
        warn("inode_get: no empty mem_inode slot");
        spinlock_release(&icache_lock);
        return NULL;
    }

    m_inode = empty_slot;
    m_inode->inumber = inumber;
    m_inode->ref_cnt = 1;
    spinlock_release(&icache_lock);

    /** Lock the inode and read from disk. */
    inode_lock(m_inode);
    bool success = boot ? block_read_at_boot((char *) &(m_inode->d_inode),
                                             DISK_ADDR_INODE(inumber),
                                             sizeof(inode_t))
                        : block_read((char *) &(m_inode->d_inode),
                                     DISK_ADDR_INODE(inumber),
                                     sizeof(inode_t));
    if (!success) {
        warn("inode_get: failed to read inode %u from disk", inumber);
        inode_unlock(m_inode);
        return NULL;
    }
    inode_unlock(m_inode);

    // _print_icache_state();
    return m_inode;
}

mem_inode_t *
inode_get(uint32_t inumber)
{
    return _inode_get(inumber, false);
}

mem_inode_t *
inode_get_at_boot(uint32_t inumber)
{
    return _inode_get(inumber, true);
}

/** Increment reference to an already-got inode. */
void
inode_ref(mem_inode_t *m_inode)
{
    spinlock_acquire(&icache_lock);
    assert(m_inode->ref_cnt > 0);
    m_inode->ref_cnt++;
    spinlock_release(&icache_lock);
}

/**
 * Put down a reference to an inode. If the reference count goes to
 * zero, this icache slot becomes empty.
 */
void
inode_put(mem_inode_t *m_inode)
{
    spinlock_acquire(&icache_lock);
    assert(!parklock_holding(&(m_inode->lock)));
    assert(m_inode->ref_cnt > 0);
    m_inode->ref_cnt--;
    spinlock_release(&icache_lock);
}


/** Flush an in-memory modified inode to disk. */
static bool
_flush_inode(mem_inode_t *m_inode)
{
    return block_write((char *) &(m_inode->d_inode),
                       DISK_ADDR_INODE(m_inode->inumber),
                       sizeof(inode_t));
}

/** Allocate an inode structure on disk (and gets into memory). */
mem_inode_t *
inode_alloc(uint32_t type)
{
    /** Get a free slot according to bitmap. */
    uint32_t inumber = bitmap_alloc(&inode_bitmap);
    if (inumber == inode_bitmap.slots) {
        warn("inode_alloc: no free inode slot left");
        return NULL;
    }

    inode_t d_inode;
    memset(&d_inode, 0, sizeof(inode_t));
    d_inode.type = type;
    
    /** Persist to disk: bitmap first, then the inode. */
    if (!inode_bitmap_update(inumber)) {
        warn("inode_alloc: failed to persist inode bitmap");
        bitmap_clear(&inode_bitmap, inumber);
        return NULL;
    }

    if (!block_write((char *) &d_inode,
                     DISK_ADDR_INODE(inumber),
                     sizeof(inode_t))) {
        warn("inode_alloc: failed to persist inode %u", inumber);
        bitmap_clear(&inode_bitmap, inumber);
        inode_bitmap_update(inumber);   /** Ignores error. */
        return NULL;
    }

    return inode_get(inumber);
}

/**
 * Free an on-disk inode structure (removing a file). Avoids calling
 * `_walk_inode_index()` repeatedly.
 * Must be called with lock on M_INODE held.
 */
void
inode_free(mem_inode_t *m_inode)
{
    m_inode->d_inode.size = 0;
    m_inode->d_inode.type = 0;

    /** Direct. */
    for (size_t idx0 = 0; idx0 < NUM_DIRECT; ++idx0) {
        if (m_inode->d_inode.data0[idx0] != 0) {
            block_free(m_inode->d_inode.data0[idx0]);
            m_inode->d_inode.data0[idx0] = 0;
        }
    }
    
    /** Singly-indirect. */
    for (size_t idx0 = 0; idx0 < NUM_INDIRECT1; ++idx0) {
        uint32_t ib1_addr = m_inode->d_inode.data1[idx0];
        if (ib1_addr != 0) {
            uint32_t ib1[UINT32_PB];
            if (block_read((char *) ib1, ib1_addr, BLOCK_SIZE)) {
                for (size_t idx1 = 0; idx1 < UINT32_PB; ++idx1) {
                    if (ib1[idx1] != 0)
                        block_free(ib1[idx1]);
                }
            }
            block_free(ib1_addr);
            m_inode->d_inode.data1[idx0] = 0;
        }
    }

    /** Doubly-indirect. */
    for (size_t idx0 = 0; idx0 < NUM_INDIRECT2; ++idx0) {
        uint32_t ib1_addr = m_inode->d_inode.data2[idx0];
        if (ib1_addr != 0) {
            uint32_t ib1[UINT32_PB];
            if (block_read((char *) ib1, ib1_addr, BLOCK_SIZE)) {
                for (size_t idx1 = 0; idx1 < UINT32_PB; ++idx1) {
                    uint32_t ib2_addr = ib1[idx1];
                    if (ib2_addr != 0) {
                        uint32_t ib2[UINT32_PB];
                        if (block_read((char *) ib2, ib2_addr, BLOCK_SIZE)) {
                            for (size_t idx2 = 0; idx2 < UINT32_PB; ++idx2) {
                                if (ib2[idx2] != 0)
                                    block_free(ib2[idx2]);
                            }
                        }
                        block_free(ib1[idx1]);
                    }
                }
            }
            block_free(ib1_addr);
            m_inode->d_inode.data2[idx0] = 0;
        }
    }

    _flush_inode(m_inode);

    bitmap_clear(&inode_bitmap, m_inode->inumber);
    inode_bitmap_update(m_inode->inumber);      /** Ignores error. */
}


/**
 * Walk the indexing array to get block number for the n-th block.
 * Allocates the block if was not allocated. Returns address 0
 * (which is invalid for a data block) on failures.
 */
static uint32_t
_walk_inode_index(mem_inode_t *m_inode, uint32_t idx)
{
    /** Direct. */
    if (idx < NUM_DIRECT) {
        if (m_inode->d_inode.data0[idx] == 0)
            m_inode->d_inode.data0[idx] = block_alloc();
        return m_inode->d_inode.data0[idx];
    }
    
    /** Singly-indirect. */
    idx -= NUM_DIRECT;
    if (idx < NUM_INDIRECT1 * UINT32_PB) {
        size_t idx0 = idx / UINT32_PB;
        size_t idx1 = idx % UINT32_PB;

        /** Load indirect1 block. */
        uint32_t ib1_addr = m_inode->d_inode.data1[idx0];
        if (ib1_addr == 0) {
            ib1_addr = block_alloc();
            if (ib1_addr == 0)
                return 0;
            m_inode->d_inode.data1[idx0] = ib1_addr;
        }
        uint32_t ib1[UINT32_PB];
        if (!block_read((char *) ib1, ib1_addr, BLOCK_SIZE))
            return 0;

        /** Index in the indirect1 block. */
        if (ib1[idx1] == 0) {
            ib1[idx1] = block_alloc();
            if (ib1[idx1] == 0)
                return 0;
            if (!block_write((char *) ib1, ib1_addr, BLOCK_SIZE))
                return 0;
        }

        return ib1[idx1];
    }

    /** Doubly indirect. */
    idx -= NUM_INDIRECT1 * UINT32_PB;
    if (idx < NUM_INDIRECT2 * UINT32_PB*UINT32_PB) {
        size_t idx0 = idx / (UINT32_PB*UINT32_PB);
        size_t idx1 = (idx % (UINT32_PB*UINT32_PB)) / UINT32_PB;
        size_t idx2 = idx % UINT32_PB;

        /** Load indirect1 block. */
        uint32_t ib1_addr = m_inode->d_inode.data2[idx0];
        if (ib1_addr == 0) {
            ib1_addr = block_alloc();
            if (ib1_addr == 0)
                return 0;
            m_inode->d_inode.data2[idx0] = ib1_addr;
        }
        uint32_t ib1[UINT32_PB];
        if (!block_read((char *) ib1, ib1_addr, BLOCK_SIZE))
            return 0;

        /** Load indirect2 block. */
        uint32_t ib2_addr = ib1[idx1];
        if (ib2_addr == 0) {
            ib2_addr = block_alloc();
            if (ib2_addr == 0)
                return 0;
            ib1[idx1] = ib2_addr;
            if (!block_write((char *) ib1, ib1_addr, BLOCK_SIZE))
                return 0;
        }
        uint32_t ib2[UINT32_PB];
        if (!block_read((char *) ib2, ib2_addr, BLOCK_SIZE))
            return 0;

        /** Index in the indirect2 block. */
        if (ib2[idx2] == 0) {
            ib2[idx2] = block_alloc();
            if (ib2[idx2] == 0)
                return 0;
            if (!block_write((char *) ib2, ib2_addr, BLOCK_SIZE))
                return 0;
        }

        return ib2[idx2];
    }

    warn("walk_inode_index: index %u is out of range", idx);
    return 0;
}

/**
 * Read data at logical offset from inode. Returns the number of bytes
 * actually read.
 * Must with lock on M_INODE held.
 */
size_t
inode_read(mem_inode_t *m_inode, char *dst, uint32_t offset, size_t len)
{
    if (offset > m_inode->d_inode.size)
        return 0;
    if (offset + len > m_inode->d_inode.size)
        len = m_inode->d_inode.size - offset;

    uint32_t bytes_read = 0;
    while (len > bytes_read) {
        uint32_t bytes_left = len - bytes_read;

        uint32_t start_offset = offset + bytes_read;
        uint32_t req_offset = ADDR_BLOCK_OFFSET(start_offset);

        uint32_t next_offset = ADDR_BLOCK_ROUND_DN(start_offset) + BLOCK_SIZE;
        uint32_t effective = next_offset - start_offset;
        if (bytes_left < effective)
            effective = bytes_left;

        uint32_t block_addr = _walk_inode_index(m_inode, start_offset / BLOCK_SIZE);
        if (block_addr == 0) {
            warn("inode_read: failed to walk inode index on offset %u", start_offset);
            return bytes_read;
        }

        if (!block_read(dst + bytes_read, block_addr + req_offset, effective)) {
            warn("inode_read: failed to read disk address %p", block_addr);
            return bytes_read;
        }

        bytes_read += effective;
    }

    return bytes_read;
}

/**
 * Write data at logical offset of inode. Returns the number of bytes
 * actually written. Will extend the inode if the write exceeds current
 * file size.
 * Must with lock on M_INODE held.
 */
size_t
inode_write(mem_inode_t *m_inode, char *src, uint32_t offset, size_t len)
{
    if (offset > m_inode->d_inode.size)
        return 0;

    uint32_t bytes_written = 0;
    while (len > bytes_written) {
        uint32_t bytes_left = len - bytes_written;

        uint32_t start_offset = offset + bytes_written;
        uint32_t req_offset = ADDR_BLOCK_OFFSET(start_offset);

        uint32_t next_offset = ADDR_BLOCK_ROUND_DN(start_offset) + BLOCK_SIZE;
        uint32_t effective = next_offset - start_offset;
        if (bytes_left < effective)
            effective = bytes_left;

        uint32_t block_addr = _walk_inode_index(m_inode, start_offset / BLOCK_SIZE);
        if (block_addr == 0) {
            warn("inode_write: failed to walk inode index on offset %u", start_offset);
            return bytes_written;
        }

        if (!block_write(src + bytes_written, block_addr + req_offset, effective)) {
            warn("inode_write: failed to write block address %p", block_addr);
            return bytes_written;
        }

        bytes_written += effective;
    }

    /** Update inode size if extended. */
    if (offset + len > m_inode->d_inode.size) {
        m_inode->d_inode.size = offset + len;
        _flush_inode(m_inode);
    }

    return bytes_written;
}


/** Allocate a slot in the opne file table. Returns NULL on failure. */
file_t *
file_get(void)
{
    spinlock_acquire(&ftable_lock);

    for (file_t *file = ftable; file < &ftable[MAX_OPEN_FILES]; ++file) {
        if (file->ref_cnt == 0) {
            file->ref_cnt = 1;

            spinlock_release(&ftable_lock);
            return file;
        }
    }

    spinlock_release(&ftable_lock);
    return NULL;
}

/** Increment reference to an already-got file. */
void
file_ref(file_t *file)
{
    spinlock_acquire(&ftable_lock);
    assert(file->ref_cnt > 0);
    file->ref_cnt++;
    spinlock_release(&ftable_lock);
}

/**
 * Put down a reference to an open file. If the reference count goes to
 * zero, actually closes this file and this ftable slot becomes empty.
 */
void
file_put(file_t *file)
{
    mem_inode_t *inode;

    /** Decrement reference count. */
    spinlock_acquire(&ftable_lock);
    assert(file->ref_cnt > 0);
    
    file->ref_cnt--;
    if (file->ref_cnt > 0) {    /** Do nothing if still referenced. */
        spinlock_release(&ftable_lock);
        return;
    }

    inode = file->inode;        /** Remember inode for putting. */
    spinlock_release(&ftable_lock);

    /** Actually closing, put inode. */
    inode_put(inode);
}


/** Get metadata information of a file. */
void
file_stat(file_t *file, file_stat_t *stat)
{
    inode_lock(file->inode);

    stat->inumber = file->inode->inumber;
    stat->type = file->inode->d_inode.type;
    stat->size = file->inode->d_inode.size;

    inode_unlock(file->inode);
}
