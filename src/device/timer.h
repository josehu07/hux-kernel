/**
 * Programmable interval timer (PIT) in square wave generator mode to
 * serve as the system timer.
 */


#ifndef TIMER_H
#define TIMER_H


#include <stdint.h>


/** Timer interrupt frequency in Hz. */
#define TIMER_FREQ_HZ 100


void timer_init();


#endif
