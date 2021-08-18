/**
 * Block-level I/O general request layer.
 */


#ifndef BLOCK_H
#define BLOCK_H


#include <stdint.h>
#include <stdbool.h>


/** All block requests are of size 1024 bytes. */
#define BLOCK_SIZE 1024


/** Helper macros on addresses and block alignments. */
#define ADDR_BLOCK_OFFSET(addr) ((addr) & 0x000003FF)
#define ADDR_BLOCK_NUMBER(addr) ((addr) >> 10)

#define ADDR_BLOCK_ALIGNED(addr) (ADDR_BLOCK_OFFSET(addr) == 0)

#define ADDR_BLOCK_ROUND_DN(addr) ((addr) & 0xFFFFFC00)
#define ADDR_BLOCK_ROUND_UP(addr) (ADDR_BLOCK_ROUND_DN((addr) + 0x000003FF))


/**
 * Block device request buffer.
 *   - valid && dirty:   waiting to be written to disk
 *   - !valid && !dirty: waiting to be read from disk
 *   - valid && !dirty:  normal buffer with valid data
 *   - !valid && dirty:  cannot happen
 */
struct block_request {
    bool valid;
    bool dirty;
    struct block_request *next;     /** Next in device queue. */
    uint32_t block_no;              /** Block index on disk. */
    uint8_t data[BLOCK_SIZE];
};
typedef struct block_request block_request_t;


bool block_read(char *dst, uint32_t disk_addr, uint32_t len);
bool block_read_at_boot(char *dst, uint32_t disk_addr, uint32_t len);
bool block_write(char *src, uint32_t disk_addr, uint32_t len);

uint32_t block_alloc();
void block_free(uint32_t disk_addr);


#endif
