/**
 * Block I/O request information structure.
 */


#ifndef BLOCK_H
#define BLOCK_H


/** All block requests are of size 1024 bytes. */
#define BLOCK_SIZE 1024


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


#endif
