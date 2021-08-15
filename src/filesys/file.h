/**
 * In-memory structures and operations on open files.
 */


#ifndef FILE_H
#define FILE_H


#include <stdint.h>
#include <stdbool.h>

#include "vsfs.h"


/** In-memory copy of open inode, be sure struct size <= 128 bytes. */
struct mem_inode {
    uint8_t ref_cnt;    /** Reference count (from file handles). */
    uint32_t inumber;   /** Inode number identifier. */
    /** Read in on-disk inode information. */
    uint32_t data0[NUM_DIRECT];     /** Direct blocks. */
    uint32_t data1[NUM_INDIRECT1];  /** 1-level indirect blocks. */
    uint32_t data2[NUM_INDIRECT2];  /** 2-level indirect blocks. */
    uint32_t size;                  /** File size in bytes. */
};
typedef struct mem_inode mem_inode_t;

/** Maximum number of in-memory cached inodes. */
#define MAX_MEM_INODES 100


/** Open file handle structure. */
struct file {
    uint8_t ref_cnt;        /** Reference count (from forked processes). */
    bool readable;          /** Open as readable? */
    bool writable;          /** Open as writable? */
    mem_inode_t *inode;     /** Inode structure of the file. */
    uint32_t offset;        /** Current file offset in bytes. */
};
typedef struct file file_t;

/** Maximum number of open files in the system. */
#define MAX_OPEN_FILES 200


/** Extern the tables to `vsfs.c`. */
extern mem_inode_t icache[];
extern file_t ftable[];


#endif
