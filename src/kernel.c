/**
 * The Hux kernel entry point.
 */


/** Check correct cross compiling. */
#if !defined(__i386__)
#error "The Hux kernel needs to be compiled with an 'ix86-elf' compiler"
#endif


#include "common/printf.h"
#include "display/terminal.h"


static int static_var;


/** The main function that `boot.s` jumps to. */
void
kernel_main(void)
{
    terminal_init();

    int stack_var;

    tprintf("[%+#010x], [%X], [%-+ #zu], [%0 5li], [%-7d], [%#ho], [%#b]\n",
            &static_var, &stack_var, sizeof(void *), (long) 791, -238, (short) 11, 13);

    tprintf("[%0+10.4lf], [%.3f], [%-10lf], [% 8F], [%#F]\n",
            37.9, -29086.008446435, 0.27121759, -3.14159, 2.0000718);

    tprintf("[%3c], [%-5c], [%c] | ", 'H', 'u', 'X');
    tprintf("[%3s], [%-7s], [%s]\n", "hux-kernel", "Hux", "Kernel");

    tprintf("[%p], [%p], [%%]\n", &static_var, &stack_var);

    tprintf("%-#0t, %123-d, %m ... - These are invalid!\n");

    char buf[100];

    sprintf(buf, "Stack pointer: %p\n", &stack_var);
    tprintf("Buf contains: %s", buf);
}
