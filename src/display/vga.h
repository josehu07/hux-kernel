/**
 * VGA text mode specifications.
 */


#ifndef VGA_H
#define VGA_H


#include <stdint.h>


/** Hardcoded 4-bit color codes. */
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


/**
 * VGA entry composer.
 * A VGA entry = [4bits bg | 4bits fg | 8bits content].
 */
static inline uint16_t
vga_entry(vga_color_t bg, vga_color_t fg, unsigned char c)
{
    return (uint16_t) c | (uint16_t) fg << 8 | (uint16_t) bg << 12;
}


#endif
