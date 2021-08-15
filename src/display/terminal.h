/**
 * Terminal display control.
 */


#ifndef TERMINAL_H
#define TERMINAL_H


#include <stddef.h>

#include "vga.h"

#include "../common/spinlock.h"


/** Extern to `printf.c` and other places of calling `terminal_`. */
extern spinlock_t terminal_lock;


/**
 * Default to black background + light grey foreground.
 * Foreground color can be customized with '*_color' functions.
 */
extern const vga_color_t TERMINAL_DEFAULT_COLOR_BG;
extern const vga_color_t TERMINAL_DEFAULT_COLOR_FG;


void terminal_init();

void terminal_write(const char *data, size_t size);
void terminal_write_color(const char *data, size_t size, vga_color_t fg);

void terminal_erase();
void terminal_clear();


#endif
