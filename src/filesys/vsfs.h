/**
 * Very simple file system (VSFS) data structures & Layout.
 */


#ifndef VSFS_H
#define VSFS_H


#include <stdint.h>
#include <stdbool.h>


/** Root directory must be the 0-th inode. */
#define ROOT_INUMBER 0


/**
 * VSFS of Hux has the following on-disk layout:
 * 
 *   - Block 0 is the superblock holding meta information of the FS;
 *   - Block 1 is the inode slots bitmap;
 *   - Block 2 is the data blocks bitmap;
 *   - Blocks 3 - 6143 (upto 6 MiB offset) are inode blocks;
 *   - All the rest blocks up to 256 MiB are data blocks.
 * 
 *   * Block size is 1 KiB = 2 disk sectors
 *   * Inode structure is 128 bytes, so an inode block has 8 slots
 *   * File system size is 256 MiB = 262144 blocks
 *   * Inode 0 is the root path directory "/"
 *
 * The mkfs script builds an initial VSFS disk image which should follow
 * the above description.
 */
struct superblock {
    uint32_t fs_blocks;             /** Should be 262144. */
    uint32_t inode_bitmap_start;    /** Should be 1. */
    uint32_t inode_bitmap_blocks;   /** Should be 6. */
    uint32_t data_bitmap_start;     /** Should be 7. */
    uint32_t data_bitmap_blocks;    /** Should be 32. */
    uint32_t inode_start;           /** Should be 39. */
    uint32_t inode_blocks;          /** Should be 6105. */
    uint32_t data_start;            /** Should be 6144. */
    uint32_t data_blocks;           /** Should be 256000. */
} __attribute__((packed));
typedef struct superblock superblock_t;

/** Extern the superblock for info printing. */
extern superblock_t superblock;


/**
 * An inode points to 16 direct blocks, 8 singly-indirect blocks, and
 * 1 doubly-indirect block. With an FS block size of 1KB, the maximum
 * file size = 1 * 256^2 + 8 * 256 + 16 KiB = 66 MiB 16 KiB.
 */
#define NUM_DIRECT    16
#define NUM_INDIRECT1 8
#define NUM_INDIRECT2 1

#define UINT32_PB (BLOCK_SIZE / 4)
#define FILE_MAX_BLOCKS (NUM_INDIRECT2 * UINT32_PB*UINT32_PB \
                         + NUM_INDIRECT1 * UINT32_PB         \
                         + NUM_DIRECT)

/** On-disk inode structure of exactly 128 bytes in size. */
#define INODE_SIZE 128

#define INODE_TYPE_EMPTY 0
#define INODE_TYPE_FILE  1
#define INODE_TYPE_DIR   2

struct inode {
    uint32_t type;                  /** 0 = empty, 1 = file, 2 = directory. */
    uint32_t size;                  /** File size in bytes. */
    uint32_t data0[NUM_DIRECT];     /** Direct blocks. */
    uint32_t data1[NUM_INDIRECT1];  /** 1-level indirect blocks. */
    uint32_t data2[NUM_INDIRECT2];  /** 2-level indirect blocks. */
    /** Rest bytes are unused. */
} __attribute__((packed));
typedef struct inode inode_t;


/** Helper macros for calculating on-disk address. */
#define DISK_ADDR_INODE(i) (superblock.inode_start * BLOCK_SIZE + (i) * INODE_SIZE)
#define DISK_ADDR_DATA_BLOCK(d) ((superblock.data_start + (d)) * BLOCK_SIZE)


/**
 * Directory entry structure. A directory's data is simply a contiguous
 * array of such 128-bytes entry structures.
 */
#define DENTRY_SIZE 128

#define MAX_FILENAME 120

struct dentry {
    uint32_t inumber;   /** 0 means not used (root path directory "/" can't be in
                            any directory without the support of linking). */
    char filename[DENTRY_SIZE - sizeof(uint32_t)];      /** String name. */
} __attribute__((packed));
typedef struct dentry dentry_t;


void filesys_init();


#endif
