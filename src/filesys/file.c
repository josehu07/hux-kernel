/**
 * In-memory structures and operations on open files.
 */


#include <stdint.h>
#include <stdbool.h>

#include "file.h"


/** Open file table - list of open file structures. */
file_t ftable[MAX_OPEN_FILES];

/** Open inode table - list of in-memory inode structures. */
mem_inode_t icache[MAX_MEM_INODES];


/**
 * Get inode for given inode number. If that inode has been in memory,
 * increment its ref count and return. Otherwise, read from the disk into
 * an empty inode cache slot.
 */
mem_inode_t *
inode_get(uint32_t inumber)
{
    mem_inode_t *inode = NULL;

    cli_push();

    /** Search icache to see if it has been in memory. */
    mem_inode_t *empty_slot = NULL;
    for (inode = icache; inode < &icache[MAX_MEM_INODES]; ++inode) {
        if (inode->ref_cnt > 0 && inode->inumber == inumber) {
            inode->ref_cnt++;
            cli_pop();
            return inode;
        }
        if (empty_slot == MAX_MEM_INODES && inode->ref_cnt == 0)
            empty_slot = inode;     /** Remember empty slot seen. */
    }

    if (empty_slot == NULL) {
        warn("inode_get: no empty mem_inode slot");
        cli_pop();
        return NULL;
    }

    inode = empty_slot;
    inode->inumber = inumber;
    inode->ref_cnt = 1;

    /** Read from disk. */
    parklock_acquire(inode->lock);
    // TODO
}

// void
// ilock(struct inode *ip)
// {
//   struct buf *bp;
//   struct dinode *dip;

//   if(ip == 0 || ip->ref < 1)
//     panic("ilock");

//   acquiresleep(&ip->lock);

//   if(ip->valid == 0){
//     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
//     dip = (struct dinode*)bp->data + ip->inum%IPB;
//     ip->type = dip->type;
//     ip->major = dip->major;
//     ip->minor = dip->minor;
//     ip->nlink = dip->nlink;
//     ip->size = dip->size;
//     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
//     brelse(bp);
//     ip->valid = 1;
//     if(ip->type == 0)
//       panic("ilock: no type");
//   }
// }
