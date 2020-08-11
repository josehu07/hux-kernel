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

static int static_var;
/** The main function that `boot.s` jumps to. */
// void
// kernel_main(unsigned long magic, unsigned long addr)
// {
//     /** Initialize VGA text-mode terminal support. */
//     terminal_init();

//     /** Double check the multiboot magic number. */
//     if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
//         prompt_error();
//         tprintf("Invalid bootloader magic: %#x\n", magic);
//         return;
//     }

//     /** Get pointer to multiboot info. */
//     multiboot_info_t *mbi = (multiboot_info_t *) addr;

//     /** Initialize debugging utilities. */
//     debug_init(mbi);

//     assert(0);

//     tprintf("This line should not be displayed!");
// }
void
kernel_main(void)
{
    terminal_init();

    int stack_var;

    printf("[%+#010x], [%X], [%-+ #zu], [%0 5li], [%-7d], [%#ho], [%#b]\n",
           &static_var, &stack_var, sizeof(void *), (long) 791, -238, (short) 11, 13);

    cprintf(VGA_COLOR_GREEN, "[%0+10.4lf], [%.3f], [%-10lf], [% 8F], [%#F]\n",
            37.9, -29086.008446435, 0.27121759, -3.14159, 2.0000718);

    printf("[%3c], [%-5c], [%c] | ", 'H', 'u', 'X');
    cprintf(VGA_COLOR_CYAN, "[%3s], [%-7s], [%s]\n", "hux-kernel", "Hux", "Kernel");

    printf("[%p], [%p], [%%]\n", &static_var, &stack_var);

    printf("%-#0t, %123-d, %m ... - These are invalid!\n");

    char buf[100];

    snprintf(buf, 99, "Stack pointer: %p\n", &stack_var);
    cprintf(VGA_COLOR_BLUE, "Buf contains: %s", buf);
}
