/**
 * The Hux kernel - entry point, following the OSDev wiki on
 * https://wiki.osdev.org/Bare_Bones.
 */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/** Check correct cross compiling. */
#if !defined(__i386__)
#error "The Hux kernel needs to be compiled with an 'ix86-elf' compiler"
#endif


/**
 * Minimal VGA terminal display support.
 * VGA text mode display buffer is at address 0xB8000 by specification.
 */
uint16_t *VGA_BUFFER = (uint16_t *) 0xB8000;

static const size_t VGA_WIDTH  = 80;
static const size_t VGA_HEIGHT = 25;
static const uint8_t VGA_COLOR = 0 << 4 | 15;   /** Black bg | White fg. */

/** A VGA entry (here, a char) = 4bits bg | 4bits fg | 8bits content. */
static inline uint16_t
vga_entry(unsigned char c)
{
    return (uint16_t) c | (uint16_t) VGA_COLOR << 8;
}

/** Current terminal position. */
size_t terminal_row;
size_t terminal_col;

void
terminal_initialize(void)
{
    terminal_row = 0;
    terminal_col = 0;

    /** Flush the window to be all spaces. */
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_BUFFER[y * VGA_WIDTH + x] = vga_entry(' ');
        }
    }
}

void
terminal_putchar(char c)
{
    VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_col] = vga_entry(c);

    /** When cursor hits the window boundary. */
    if (++terminal_col == VGA_WIDTH) {
        terminal_col = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

void
terminal_writestring(const char *str)
{
    /** Calculate string length. */
    size_t len = 0;
    while (str[len])
        len++;

    for (size_t i = 0; i < len; ++i)
        terminal_putchar(str[i]);
}


/** The main function that `boot.s` jumps to. */
void
kernel_main(void)
{
    terminal_initialize();
    terminal_writestring("Hello, world!");
}
