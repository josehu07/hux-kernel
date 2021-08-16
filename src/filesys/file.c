/**
 * In-memory structures and operations on open files.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "file.h"
#include "block.h"
#include "vsfs.h"

#include "../common/debug.h"
#include "../common/spinlock.h"
#include "../common/parklock.h"


/** Open inode table - list of in-memory inode structures. */
mem_inode_t icache[MAX_MEM_INODES];
spinlock_t icache_lock;

/** Open file table - list of open file structures. */
file_t ftable[MAX_OPEN_FILES];
spinlock_t ftable_lock;


/**
 * Get inode for given inode number. If that inode has been in memory,
 * increment its ref count and return. Otherwise, read from the disk into
 * an empty inode cache slot.
 *
 * Returns with the lock on this mem_inode held.
 */
mem_inode_t *
inode_get(uint32_t inumber)
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
    parklock_acquire(&(m_inode->lock));
    if (!block_read((char *) &(m_inode->d_inode), DISK_ADDR_INODE(inumber),
                    sizeof(inode_t), false)) {
        warn("inode_get: failed to read inode %u from disk", inumber);
        return NULL;
    }

    return m_inode;
}

/**
 * Put down a reference to an inode. If the reference count goes to
 * zero, this icache slot becomes empty.
 */
void
inode_put(mem_inode_t *m_inode)
{
    assert(m_inode != NULL);
    assert(parklock_holding(&(m_inode->lock)));
    assert(m_inode->ref_cnt >= 1);
    assert(m_inode->d_inode.type != INODE_TYPE_EMPTY);

    parklock_release(&(m_inode->lock));

    spinlock_acquire(&icache_lock);
    m_inode->ref_cnt--;
    spinlock_release(&icache_lock);
}
