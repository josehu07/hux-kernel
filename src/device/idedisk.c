/**
 * Parallel ATA (IDE) hard disk driver.
 * Assumes only port I/O (PIO) mode without DMA for now.
 */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "idedisk.h"

#include "../common/port.h"
#include "../common/debug.h"
#include "../common/string.h"
#include "../common/intstate.h"

#include "../interrupt/isr.h"

#include "../filesys/block.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** Data returned by the IDENTIFY command during initialization. */
static uint16_t ide_identify_data[256];


/** IDE pending requests software queue. */
static block_request_t *ide_queue_head = NULL;
static block_request_t *ide_queue_tail = NULL;


/**
 * Wait for IDE disk on primary bus to become ready. Returns false on errors
 * or device faults, otherwise true.
 */
static bool
idedisk_wait_ready(void)
{
    uint8_t status;
    do {
        /** Read from alternative status so it won't affect interrupts. */
        status = inb(IDE_PORT_R_ALT_STATUS);
    } while ((status & (IDE_STATUS_BSY | IDE_STATUS_RDY)) != IDE_STATUS_RDY);

    if ((status & (IDE_STATUS_DF | IDE_STATUS_ERR)) != 0)
        return false;
    return true;
}


/**
 * Start a request to IDE disk.
 * Must be called with interrupts off.
 */
static void
idedisk_start_req(block_request_t *req)
{
    assert(req != NULL);
    // if (req->block_no >= FILESYS_SIZE)
    //     error("idedisk: request block number exceeds file system size");

    uint8_t sectors_per_block = BLOCK_SIZE / IDE_SECTOR_SIZE;
    uint32_t sector_no = req->block_no * sectors_per_block;

    /** Wait for disk to be in ready state. */
    idedisk_wait_ready();

    outb(IDE_PORT_RW_SECTORS, sectors_per_block);            /** Number of sectors. */
    outb(IDE_PORT_RW_LBA_LO,  sector_no         & 0xFF);   /** LBA address - low  bits. */
    outb(IDE_PORT_RW_LBA_MID, (sector_no >> 8)  & 0xFF);   /** LBA address - mid  bits. */
    outb(IDE_PORT_RW_LBA_HI,  (sector_no >> 16) & 0xFF);   /** LBA address - high bits. */
    outb(IDE_PORT_RW_SELECT, ide_select_entry(true, 0, sector_no)); /** LBA bits 24-27. */

    /** If dirty, kick off a write, otherwise kick off a read. */
    if (req->dirty) {
        outb(IDE_PORT_W_COMMAND, (sectors_per_block == 1) ? IDE_CMD_WRITE
                                                          : IDE_CMD_WRITE_MULTIPLE);
        /** Must be a stream in 32-bit dwords, can't be in 8-bit bytes. */
        outsl(IDE_PORT_RW_DATA, req->data, BLOCK_SIZE / sizeof(uint32_t));
    } else {
        outb(IDE_PORT_W_COMMAND, (sectors_per_block == 1) ? IDE_CMD_READ
                                                          : IDE_CMD_READ_MULTIPLE);
    }
}


/** IDE disk interrupt handler registered for IRQ # 14. */
static void
idedisk_interrupt_handler(interrupt_state_t *state)
{
    (void) state;   /** Unused. */

    /** Head of queue is the active request currently on the fly. */
    block_request_t *req = ide_queue_head;
    if (req == NULL)
        return;

    ide_queue_head = ide_queue_head->next;

    /** If is a read, get data. */
    if (!req->dirty) {
        if (idedisk_wait_ready()) {
            /** Must be a stream in 32-bit dwords, can't be in 8-bit bytes. */
            insl(IDE_PORT_RW_DATA, req->data, BLOCK_SIZE / sizeof(uint32_t));
            req->valid = true;
        }
    } else {
        if (idedisk_wait_ready())
            req->dirty = false;
    }

    /** Wake up the process waiting on this request. */
    for (process_t *proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
        if (proc->state == BLOCKED && proc->block_on == ON_IDEDISK
            && proc->wait_req == req) {
            proc->wait_req = NULL;
            process_unblock(proc);
        }
    }

    /** If more requests in queue, start the disk on the next one. */
    if (ide_queue_head != NULL)
        idedisk_start_req(ide_queue_head);
    else
        ide_queue_tail = NULL;
}


/**
 * Initialize a single IDE disk 0 on the default primary bus. Registers the
 * IDE request interrupt ISR handler.
 */
void
idedisk_init(void)
{
    /** Register IDE disk interrupt ISR handler. */
    isr_register(INT_NO_IDEDISK, &idedisk_interrupt_handler);

    /** Select disk 0 on primary bus and wait for it to be ready */
    outb(IDE_PORT_RW_SELECT, ide_select_entry(true, 0, 0));
    idedisk_wait_ready();

    /**
     * Detect that disk 0 on the primary ATA bus is there and is a PATA
     * (IDE) device. Utilzies the IDENTIFY command.
     */
    outb(IDE_PORT_W_CONTROL,  0);
    outb(IDE_PORT_RW_SECTORS, 0);
    outb(IDE_PORT_RW_LBA_LO,  0);
    outb(IDE_PORT_RW_LBA_MID, 0);
    outb(IDE_PORT_RW_LBA_HI,  0);
    outb(IDE_PORT_W_COMMAND, IDE_CMD_IDENTIFY);

    uint8_t status = inb(IDE_PORT_R_ALT_STATUS);
    if (status == 0)
        error("idedisk_init: drive does not exist on primary bus");
    do {
        status = inb(IDE_PORT_R_ALT_STATUS);
        if (inb(IDE_PORT_RW_LBA_MID) != 0 || inb(IDE_PORT_RW_LBA_HI) != 0)
            error("idedisk_init: drive on primary bus is not PATA");
    } while ((status & (IDE_STATUS_BSY)) != 0
             || (status & (IDE_STATUS_DRQ | IDE_STATUS_ERR)) == 0);

    if ((status & (IDE_STATUS_ERR)) != 0)
        error("idedisk_init: error returned from the IDENTIFY command");

    /** Must be a stream in 32-bit dwords. */
    memset(ide_identify_data, 0, 256 * sizeof(uint16_t));
    insl(IDE_PORT_RW_DATA, ide_identify_data, 256 * sizeof(uint16_t) / sizeof(uint32_t));

    ide_queue_head = NULL;
    ide_queue_tail = NULL;
}


/**
 * Start and wait for a block request to complete. If request is dirty,
 * write to disk, clear dirty, and set valid. Else if request is not
 * valid, read from disk into data and set valid. Returns true on success
 * and false if error appears in IDE port communications.
 */
bool
idedisk_do_req(block_request_t *req)
{
    process_t *proc = running_proc();

    if (req->valid && !req->dirty)
        error("idedisk_do_req: request valid and not dirty, nothing to do");
    if (!req->valid && req->dirty)
        error("idedisk_do_req: caught a dirty request that is not valid");

    cli_push();

    /** Append to IDE pending requests queue. */
    req->next = NULL;
    if (ide_queue_tail != NULL)
        ide_queue_tail->next = req;
    else
        ide_queue_head = req;
    ide_queue_tail = req;

    /** Start he disk device if it was idle. */
    if (ide_queue_head == req)
        idedisk_start_req(req);

    /** Wait for this request to have been served. */
    proc->wait_req = req;
    process_block(ON_IDEDISK);

    /**
     * Could be re=scheduld when an IDE interrupt comes saying that this
     * request has been served. If valid is not set at this time, it means
     * error occurred.
     */
    if (!req->valid || req->dirty) {
        warn("idedisk_do_req: error occurred in IDE disk request");
        cli_pop();
        return false;
    }

    cli_pop();
    return true;
}
