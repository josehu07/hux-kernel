/**
 * In-memory structures and operations on open files.
 */


#ifndef FILE_H
#define FILE_H


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "vsfs.h"
#include "sysfile.h"

#include "../common/spinlock.h"
#include "../common/parklock.h"


/** In-memory copy of open inode, be sure struct size <= 128 bytes. */
struct mem_inode {
    uint8_t ref_cnt;    /** Reference count (from file handles). */
    uint32_t inumber;   /** Inode number identifier of `d_inode`. */
    parklock_t lock;    /** Parking lock held when waiting for disk I/O. */
    inode_t d_inode;    /** Read in on-disk inode structure. */
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

/** Maximum number of open files per process. */
#define MAX_FILES_PER_PROC 16


/** Extern the tables to `vsfs.c`. */
extern mem_inode_t icache[];
extern spinlock_t icache_lock;

extern file_t ftable[];
extern spinlock_t ftable_lock;


void inode_lock(mem_inode_t *m_inode);
void inode_unlock(mem_inode_t *m_inode);

mem_inode_t *inode_get(uint32_t inumber);
mem_inode_t *inode_get_at_boot(uint32_t inumber);
void inode_ref(mem_inode_t *m_inode);
void inode_put(mem_inode_t *m_inode);

mem_inode_t *inode_alloc(uint32_t type);
void inode_free(mem_inode_t *m_inode);

size_t inode_read(mem_inode_t *m_inode, char *dst, uint32_t offset, size_t len);
size_t inode_write(mem_inode_t *m_inode, char *src, uint32_t offset, size_t len);

file_t *file_get();
void file_ref(file_t *file);
void file_put(file_t *file);

void file_stat(file_t *file, file_stat_t *stat);


#endif
