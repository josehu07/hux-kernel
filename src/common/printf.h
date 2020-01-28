/**
 * Common formatted printing utilities.
 */


#ifndef PRINTF_H
#define PRINTF_H


#include "../display/terminal.h"


void tprintf(const char *fmt, ...);
void sprintf(char *buf, const char *fmt, ...);


/** Message header prompts. */
inline void
prompt_error()
{
    terminal_write_color("ERROR ", 6, VGA_COLOR_RED);
}

inline void
prompt_panic()
{
    terminal_write_color("PANIC ", 6, VGA_COLOR_MAGENTA);
}


#endif
