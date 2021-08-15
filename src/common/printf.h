/**
 * Common formatted printing utilities.
 *
 * Format specifier := %[special][width][.precision][length]<type>. Only
 * limited features are provided (documented in the wiki page).
 */


#ifndef PRINTF_H
#define PRINTF_H


#include <stdbool.h>

#include "../display/vga.h"
#include "../display/terminal.h"


void printf(const char *fmt, ...);
void cprintf(vga_color_t fg, const char *fmt, ...);

void snprintf(char *buf, size_t count, const char *fmt, ...);


/** Extern to `debug.h`. */
extern bool printf_to_hold_lock;


#endif
