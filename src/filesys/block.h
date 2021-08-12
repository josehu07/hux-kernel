/**
 * Block I/O request information structure.
 */


#ifndef BLOCK_H
#define BLOCK_H


/** All block requests are of size 512 bytes. */
#define BLOCK_SIZE 512


/** Block device request information. */
struct block_request {
    bool valid;
    bool dirty;
    struct block_request *next;     /** Next in device queue. */
    uint32_t block_no;
    uint8_t data[BLOCK_SIZE];
};
typedef struct block_request block_request_t;


#endif
