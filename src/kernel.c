/**
 * The Hux kernel entry point.
 */


/** Check correct cross compiling. */
#if !defined(__i386__)
#error "The Hux kernel needs to be compiled with an 'ix86-elf' compiler"
#endif


#include "common/printf.h"
#include "display/terminal.h"


/** The main function that `boot.s` jumps to. */
void
kernel_main(void)
{
    terminal_init();
}
