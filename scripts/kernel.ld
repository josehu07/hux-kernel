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
        *(.bss)
        *(COMMON)
    }
}
