/**
 * Interrupt enable/disable routines. Mimics xv6.
 */


#ifndef INTSTATE_H
#define INTSTATE_H


#include <stdbool.h>


bool interrupt_enabled(void);

void cli_push(void);
void cli_pop(void);


#endif
