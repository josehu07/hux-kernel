/**
 * Terminal display control.
 *
 * All functions called outside must have `terminal_lock` held already
 * (`printf()` does that).
 */


#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "vga.h"

#include "../common/string.h"
#include "../common/port.h"
#include "../common/debug.h"
#include "../common/spinlock.h"


static uint16_t * const VGA_MEMORY = (uint16_t *) 0xB8000;
static const size_t VGA_WIDTH  = 80;
static const size_t VGA_HEIGHT = 25;


/**
 * Default to black background + light grey foreground.
 * Foreground color can be customized with '*_color' functions.
 */
const vga_color_t TERMINAL_DEFAULT_COLOR_BG = VGA_COLOR_BLACK;
const vga_color_t TERMINAL_DEFAULT_COLOR_FG = VGA_COLOR_LIGHT_GREY;


static uint16_t *terminal_buf;
static size_t terminal_row;     /** Records current logical cursor pos. */
static size_t terminal_col;

spinlock_t terminal_lock;


/** Enable physical cursor and set thickness to 2. */
static void
_enable_cursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);  /** Start at scanline 14. */
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);  /** End at scanline 15. */
}

/** Update the actual cursor position on screen. */
static void
_update_cursor()
{
    size_t idx = terminal_row * VGA_WIDTH + terminal_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (idx & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((idx >> 8) & 0xFF));
}

/**
 * Scroll one line up, by replacing line 0-23 with the line below, and clear
 * the last line.
 */
static void
_scroll_line()
{
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t idx = y * VGA_WIDTH + x;
            if (y < VGA_HEIGHT - 1)
                terminal_buf[idx] = terminal_buf[idx + VGA_WIDTH];
            else    /** Clear the last line. */
                terminal_buf[idx] = vga_entry(TERMINAL_DEFAULT_COLOR_BG,
                                              TERMINAL_DEFAULT_COLOR_FG, ' ');
        }
    }
}

/**
 * Put a character at current cursor position with specified foreground color,
 * then update the logical cursor position. Should consider special symbols.
 */
static void
_putchar_color(char c, vga_color_t fg)
{
    switch (c) {

    case '\b':  /** Backspace. */
        if (terminal_col > 0)
            terminal_col--;
        break;

    case '\t':  /** Horizontal tab. */
        terminal_col += 4;
        terminal_col -= terminal_col % 4;
        if (terminal_col == VGA_WIDTH)
            terminal_col -= 4;
        break;

    case '\n':  /** Newline (w/ carriage return). */
        terminal_row++;
        terminal_col = 0;
        break;

    case '\r':  /** Carriage return. */
        terminal_col = 0;
        break;

    default: ;  /** Displayable character. */
        size_t idx = terminal_row * VGA_WIDTH + terminal_col;
        terminal_buf[idx] = vga_entry(TERMINAL_DEFAULT_COLOR_BG, fg, c);

        if (++terminal_col == VGA_WIDTH) {
            terminal_row++;
            terminal_col = 0;
        }
    }

    /** When going beyond the bottom line, scroll up one line. */
    if (terminal_row == VGA_HEIGHT) {
        _scroll_line();
        terminal_row--;
    }
}


/** Initialize terminal display. */
void
terminal_init(void)
{
    terminal_buf = VGA_MEMORY;
    terminal_row = 0;
    terminal_col = 0;

    spinlock_init(&terminal_lock, "terminal_lock");

    _enable_cursor();
    terminal_clear();
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
        _putchar_color(data[i], fg);
    _update_cursor();
}


/** Erase (backspace) a character. */
void
terminal_erase(void)
{
    if (terminal_col > 0)
        terminal_col--;
    else if (terminal_row > 0) {
        terminal_row--;
        terminal_col = VGA_WIDTH - 1;
    }

    size_t idx = terminal_row * VGA_WIDTH + terminal_col;
    terminal_buf[idx] = vga_entry(TERMINAL_DEFAULT_COLOR_BG,
                                  TERMINAL_DEFAULT_COLOR_FG, ' ');
    _update_cursor();
}

/** Clear the terminal window by flushing spaces. */
void
terminal_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t idx = y * VGA_WIDTH + x;
            terminal_buf[idx] = vga_entry(TERMINAL_DEFAULT_COLOR_BG,
                                          TERMINAL_DEFAULT_COLOR_FG, ' ');
        }
    }

    terminal_row = 0;
    terminal_col = 0;
    _update_cursor();
}
