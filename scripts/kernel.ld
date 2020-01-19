/*
 *  Linker script for the Hux kernel.
 */


ENTRY(start)    /* Entry @ symbol 'start'. */
SECTIONS
{
    . = 0x100000;   /* Code starts at 0x100000. */
    .text :
    {
        *(.text)
        . = ALIGN(4096);    /* Segments align to 4KB. */
    }

    .data :
    {
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
    }

    .bss :
    {
        *(.bss)
        . = ALIGN(4096);
    }

    .stab :
    {
        *(.stab)
        . = ALIGN(4096);
    }
    
    .stabstr :
    {
        *(.stabstr)
        . = ALIGN(4096);
    }

    /DISCARD/ : { *(.comment) *(.eh_frame) }
}
