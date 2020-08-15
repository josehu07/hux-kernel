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

    asm volatile ( "int $0x05" );
}
