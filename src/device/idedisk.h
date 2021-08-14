/**
 * Parallel ATA (IDE) hard disk driver.
 * Assumes only port I/O (PIO) mode without DMA for now.
 */


#ifndef IDEDISK_H
#define IDEDISK_H


#include <stdint.h>
#include <stdbool.h>

#include "../filesys/block.h"


/** Hard disk sector size is 512 bytes. */
#define IDE_SECTOR_SIZE 512


/**
 * Default I/O ports that are mapped to device registers of the IDE disk
 * on the primary bus (with I/O base = 0x1F0).
 * See https://wiki.osdev.org/ATA_PIO_Mode#Registers.
 */
#define IDE_PORT_IO_BASE        0x1F0
#define IDE_PORT_RW_DATA        (IDE_PORT_IO_BASE + 0)
#define IDE_PORT_R_ERROR        (IDE_PORT_IO_BASE + 1)
#define IDE_PORT_W_FEATURES     (IDE_PORT_IO_BASE + 1)
#define IDE_PORT_RW_SECTORS     (IDE_PORT_IO_BASE + 2)
#define IDE_PORT_RW_LBA_LO      (IDE_PORT_IO_BASE + 3)
#define IDE_PORT_RW_LBA_MID     (IDE_PORT_IO_BASE + 4)
#define IDE_PORT_RW_LBA_HI      (IDE_PORT_IO_BASE + 5)
#define IDE_PORT_RW_SELECT      (IDE_PORT_IO_BASE + 6)
#define IDE_PORT_R_STATUS       (IDE_PORT_IO_BASE + 7)
#define IDE_PORT_W_COMMAND      (IDE_PORT_IO_BASE + 7)

#define IDE_PORT_CTRL_BASE      0x3F6
#define IDE_PORT_R_ALT_STATUS   (IDE_PORT_CTRL_BASE + 0)
#define IDE_PORT_W_CONTROL      (IDE_PORT_CTRL_BASE + 0)
#define IDE_PORT_R_DRIVE_ADDR   (IDE_PORT_CTRL_BASE + 1)


/**
 * IDE error register flags (from PORT_R_ERROR).
 * See https://wiki.osdev.org/ATA_PIO_Mode#Error_Register.
 */
#define IDE_ERROR_AMNF  (1 << 0)
#define IDE_ERROR_TKZNF (1 << 1)
#define IDE_ERROR_ABRT  (1 << 2)
#define IDE_ERROR_MCR   (1 << 3)
#define IDE_ERROR_IDNF  (1 << 4)
#define IDE_ERROR_MC    (1 << 5)
#define IDE_ERROR_UNC   (1 << 6)
#define IDE_ERROR_BBK   (1 << 7)


/**
 * IDE status register flags (from PORT_R_STATUS / PORT_R_ALT_STATUS).
 * See https://wiki.osdev.org/ATA_PIO_Mode#Status_Register_.28I.2FO_base_.2B_7.29.
 */
#define IDE_STATUS_ERR (1 << 0)
#define IDE_STATUS_DRQ (1 << 3)
#define IDE_STATUS_SRV (1 << 4)
#define IDE_STATUS_DF  (1 << 5)
#define IDE_STATUS_RDY (1 << 6)
#define IDE_STATUS_BSY (1 << 7)


/**
 * IDE command codes (to PORT_W_COMMAND).
 * See https://wiki.osdev.org/ATA_Command_Matrix.
 */
#define IDE_CMD_READ           0x20
#define IDE_CMD_WRITE          0x30
#define IDE_CMD_READ_MULTIPLE  0xC4
#define IDE_CMD_WRITE_MULTIPLE 0xC5
#define IDE_CMD_IDENTIFY       0xEC


/**
 * IDE drive/head register (PORT_RW_SELECT) value.
 * See https://wiki.osdev.org/ATA_PIO_Mode#Drive_.2F_Head_Register_.28I.2FO_base_.2B_6.29.
 */
#define IDE_SELECT_DRV (1 << 4)
#define IDE_SELECT_LBA (1 << 6)

static inline uint8_t
ide_select_entry(bool use_lba, uint8_t drive, uint32_t sector_no)
{
    uint8_t reg = 0xA0;
    if (use_lba)        /** Useing LBA addressing mode. */
        reg |= IDE_SELECT_LBA;
    if (drive != 0)     /** Can only be 0 or 1 on one bus. */
        reg |= IDE_SELECT_DRV;
    reg |= (sector_no >> 24) & 0x0F;  /** LBA address, bits 24-27. */
    return reg;
}


void idedisk_init();

bool idedisk_do_req(block_request_t *req);
bool idedisk_do_req_at_boot(block_request_t *req);


#endif
