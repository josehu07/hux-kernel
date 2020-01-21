/**
 * Terminal display control utilities.
 */


#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "vga.h"

#include "../common/string.h"
#include "../common/port.h"


static uint16_t * const VGA_MEMORY = (uint16_t *) 0xB8000;
static const size_t VGA_WIDTH  = 80;
static const size_t VGA_HEIGHT = 25;

/**
 * Default to black background + white foreground.
 * Foreground color can be customized with '*_color' functions.
 */
static const vga_color_t TERMINAL_DEFAULT_COLOR_BG = VGA_COLOR_BLACK;
static const vga_color_t TERMINAL_DEFAULT_COLOR_FG = VGA_COLOR_WHITE;

static uint16_t *terminal_buf;
static size_t terminal_row;     /** Records current logical cursor pos. */
static size_t terminal_col;


/**
 * Put a character at current cursor position with specified foreground color,
 * then update the logical cursor position.
 */
static void
putchar_color(char c, vga_color_t fg)
{
    const size_t idx = terminal_row * VGA_WIDTH + terminal_col;
    terminal_buf[idx] = vga_entry(TERMINAL_DEFAULT_COLOR_BG, fg, c);

    if (++terminal_col == VGA_WIDTH) {
        terminal_col = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

/** Enable physical cursor and set thickness to 15 - 14 = 1. */
static void
enable_cursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);  /** Start at scanline 14. */
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);  /** End at scanline 15. */
}

/** Update the actual cursor position on screen. */
static void
update_cursor()
{
    const size_t idx = terminal_row * VGA_WIDTH + terminal_col;
    outb(0x3D4, 0x0F);  /** These are just standardized magic numbers. */
    outb(0x3D5, (uint8_t) (idx & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((idx >> 8) & 0xFF));
}


/** Initialize the terminal display. */
void
terminal_init(void)
{
    terminal_buf = VGA_MEMORY;
    terminal_row = 0;
    terminal_col = 0;
    terminal_clear();
    enable_cursor();
}

/** Write a sequence of data. */
void
terminal_write(const char *data, size_t size)
{
    terminal_write_color(data, size, TERMINAL_DEFAULT_COLOR_FG);
}

/** Write a sequence of data with specified foreground color. */
void
terminal_write_color(const char *data, size_t size, vga_color_t fg)
{
    for (size_t i = 0; i < size; ++i)
        putchar_color(data[i], fg);
    update_cursor();
}

/** Clear the terminal window by flushing spaces. */
void
terminal_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            const size_t idx = y * VGA_WIDTH + x;
            terminal_buf[idx] = vga_entry(TERMINAL_DEFAULT_COLOR_BG,
                                          TERMINAL_DEFAULT_COLOR_FG, ' ');
        }
    }
}
