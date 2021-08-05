/**
 * Formatted printing user library.
 *
 * Format specifier := %[special][width][.precision][length]<type>. Only
 * limited features are provided (documented in the wiki page).
 *
 * The final string CANNOT exceed 1024 bytes for every single invocation.
 */


#ifndef PRINTF_H
#define PRINTF_H


#include <stdint.h>
#include <stddef.h>


/** Hardcoded 4-bit VGA color codes. */
enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
};
typedef enum vga_color vga_color_t;

/** Default character color is light grey. */
#define PRINTF_DEFAULT_COLOR VGA_COLOR_LIGHT_GREY


void printf(const char *fmt, ...);
void cprintf(vga_color_t fg, const char *fmt, ...);

void snprintf(char *buf, size_t count, const char *fmt, ...);


#endif
