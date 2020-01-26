/**
 * Terminal display control.
 */


#ifndef TERMINAL_H
#define TERMINAL_H


#include <stddef.h>

#include "vga.h"


void terminal_init(void);

void terminal_write(const char *data, size_t size);
void terminal_write_color(const char *data, size_t size, vga_color_t fg);

void terminal_clear(void);


#endif
