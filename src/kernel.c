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
#include "common/debug.h"

#include "display/terminal.h"
#include "display/vga.h"

#include "interrupt/idt.h"

#include "memory/gdt.h"
#include "memory/paging.h"

#include "device/timer.h"
#include "device/keyboard.h"


/**
 * Enabling interrupts by executing the `sti` instruction.
 * This should be called after all devices have been initialized, so that
 * the CPU starts taking in interrupts.
 */
inline void
_enable_interrupts()
{
    asm volatile ( "sti" );
}

/** Disables interrupts by executing the `cli` instruction. */
inline void
_disable_interrupts()
{
    asm volatile ( "cli" );
}


/** Displaying initialization progress message. */
static inline void
_init_message(char *msg)
{
    printf("[");
    cprintf(VGA_COLOR_CYAN, "INIT");
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

    /** Double check the multiboot magic number. */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        error("invalid bootloader magic: %#x", magic);
        return;
    }

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
    _init_message("kicking off the PIT timer @ 100 Hz");
    timer_init(100);
    _init_message_ok();

    /** Initialize PS/2 keyboard support. */
    _init_message("initializing PS/2 keybaord support");
    keyboard_init();
    _init_message_ok();

    /** Initialize paging and switch to use paging. */
    _init_message("setting up virtual memory using paging");
    paging_init();
    _init_message_ok();
    info("supporting physical memory size: %3dMiB", NUM_FRAMES * 4 / 1024);
    info("reserving memory for the kernel: %3dMiB", KMEM_MAX / 1024 / 1024);

    /** Executes `sti`, CPU starts taking in interrupts. */
    _enable_interrupts();

    uint32_t faulty_addr = 0xBCDE0123;

    printf("\nKernel reading an unmapped address!\n");
    int dummy = *((int *) faulty_addr);
    printf("%d\n", dummy);

    while (1)   // CPU idles with a `hlt` loop.
        asm volatile ( "hlt" );
}
