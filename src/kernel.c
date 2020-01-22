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

    char *hello_str_1 = "Hello, world! ";
    for (int i = 1; i < 16; ++i)
        terminal_write_color(hello_str_1, strlen(hello_str_1), i);

    char *hello_str_2 = "Bello, del me\b\b\b\b\b\bkernel\tworld!\rH\n";
    terminal_write("\n", 1);
    for (int i = 1; i < 16; ++i)
        terminal_write_color(hello_str_2, strlen(hello_str_2), i);

    char *hello_str_3 = "Hello from Hux ;)\n";
    for (int i = 1; i < 8; ++i)
        terminal_write(hello_str_3, strlen(hello_str_3));
}
