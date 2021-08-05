/**
 * Kernel basic linker script, following the OSDev wiki on
 * https://wiki.osdev.org/Bare_Bones.
 */


/** Starts execution at the '_start' symbol as defined in `boot.s`. */
ENTRY(_start)


/** Sections layout. */
SECTIONS
{
    /**
     * Kernel's booting code will be loaded starting at 1MiB address by the
     * bootloader by convention.
     */
    . = 1M;

    .text ALIGN(4K):    /** Align to 4KiB boundary. */
    {
        KEEP(*(.multiboot))     /** Put multiboot header before code. */
        *(.text)
        *(.comment)
    }

    .rodata ALIGN(4K):
    {
        *(.rodata)
    }

    .data ALIGN(4K):
    {
        *(.data)
    }

    .bss ALIGN(4K):
    {
        *(.bss)     /** Includes our 16KiB temporary stack. */
        *(COMMON)
    }
}
