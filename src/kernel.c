/**
 * The Hux kernel entry point.
 */


/** Check correct cross compiling. */
#if !defined(__i386__)
#error "The Hux kernel needs to be compiled with an 'ix86-elf' compiler"
#endif


#include "boot/multiboot.h"
#include "boot/elf.h"

#include "common/printf.h"
#include "common/string.h"
#include "common/debug.h"

#include "display/terminal.h"
#include "display/vga.h"

#include "interrupt/idt.h"

#include "memory/gdt.h"
#include "memory/paging.h"
#include "memory/slabs.h"
#include "memory/kheap.h"

#include "process/process.h"
#include "process/scheduler.h"

#include "device/timer.h"
#include "device/keyboard.h"
#include "device/idedisk.h"

#include "filesys/block.h"
#include "filesys/vsfs.h"


/** Displaying initialization progress message. */
static inline void
_init_message(char *msg)
{
    printf("[");
    cprintf(VGA_COLOR_BLUE, "INIT");
    printf("] %s...", msg);
}

static inline void
_init_message_ok(void)
{
    cprintf(VGA_COLOR_GREEN, " OK\n");
}


/** The main function that `boot.s` jumps to. */
void
kernel_main(unsigned long magic, unsigned long addr)
{
    /** Initialize VGA text-mode terminal support. */
    terminal_init();
    _init_message("setting up VGA terminal display");
    _init_message_ok();

    /** Double check the multiboot magic number. */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        error("invalid bootloader magic: %#x", magic);

    /** Get pointer to multiboot info. */
    multiboot_info_t *mbi = (multiboot_info_t *) addr;

    /** Initialize debugging utilities. */
    _init_message("initializing debugging utilities");
    debug_init(mbi);
    _init_message_ok();

    /** Initialize global descriptor table (GDT). */
    _init_message("setting up global descriptor table (GDT)");
    gdt_init();
    _init_message_ok();

    /** Initialize interrupt descriptor table (IDT). */
    _init_message("setting up interrupt descriptor table (IDT)");
    idt_init();
    _init_message_ok();

    /** Initialize PIT timer at 100 Hz frequency. */
    _init_message("kicking off the PIT timer & interrupts");
    timer_init();
    _init_message_ok();
    info("timer frequency is set to %dHz", TIMER_FREQ_HZ);

    /** Initialize PS/2 keyboard support. */
    _init_message("initializing PS/2 keyboard support");
    keyboard_init();
    _init_message_ok();

    /** Initialize paging and switch to use paging. */
    _init_message("setting up virtual memory using paging");
    paging_init();
    _init_message_ok();
    info("supporting physical memory size: %3dMiB", NUM_FRAMES * 4 / 1024);
    info("reserving memory for the kernel: %3dMiB", KMEM_MAX / 1024 / 1024);

    /** Initialize the kernel heap allocators. */
    _init_message("initializing kernel heap memory allocators");
    page_slab_init();
    kheap_init();
    _init_message_ok();
    info("kernel page SLAB list starts at %p", PAGE_SLAB_MIN);
    info("kernel flexible heap  starts at %p", kheap_curr);

    /** Initialize CPU state, process structures, and the `init` process. */
    _init_message("initializing CPU state & process structures");
    cpu_init();
    process_init();
    _init_message_ok();
    info("maximum number of processes: %d", MAX_PROCS);

    /** Initialize IDE hard disk storage device. */
    _init_message("initializing IDE hard disk device driver");
    idedisk_init();
    _init_message_ok();

    /** Initialize the VSFS file system from disk. */
    _init_message("initializing VSFS file system from disk");
    filesys_init();
    initproc_init();
    _init_message_ok();
    info("file system block size: %u KiB", BLOCK_SIZE);
    info("file system image has %u blocks", superblock.fs_blocks);

    /** Executes `sti`, CPU starts taking in interrupts. */
    asm volatile ( "sti" );

    /**
     * Jump into the scheduler. For now, it will pick up the only ready
     * process which is `init` and context switch to it, then never
     * switching back.
     */
    terminal_clear();
    scheduler();

    error("CPU leaves the scheduler loop, should not happen");
}
