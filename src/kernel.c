/**
 * The Hux kernel entry point.
 */


/** Check correct cross compiling. */
#if !defined(__i386__)
#error "The Hux kernel needs to be compiled with an 'ix86-elf' compiler"
#endif


#include "display/terminal.h"
#include "common/string.h"


/** The main function that `boot.s` jumps to. */
void
kernel_main(void)
{
    terminal_init();

    char *hello_str = "Hello, world! 1239087";
    for (int i = 0; i < 16; ++i)
        terminal_write_color(hello_str, strlen(hello_str), i);
}
