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

#include "gdt/gdt.h"

#include "interrupt/idt.h"

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
    debug_init(mbi);

    /** Initialize global descriptor table (GDT). */
    gdt_init();

    /** Initialize interrupt descriptor table (IDT). */
    idt_init();

    /** Initialize PIT timer at 100 Hz frequency. */
    timer_init(100);

    /** Initialize PS/2 keyboard support. */
    keyboard_init();

    /** Executes `sti`, CPU starts taking in interrupts. */
    _enable_interrupts();

    while (1)   // CPU idles with a `hlt` loop.
        asm volatile ( "hlt" );
}
