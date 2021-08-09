/**
 * Programmable interval timer (PIT) in square wave generator mode to
 * serve as the system timer.
 */


#ifndef TIMER_H
#define TIMER_H


#include <stdint.h>


/** Timer interrupt frequency in Hz. */
#define TIMER_FREQ_HZ 100


/** Extern the global timer ticks value to the scheduler. */
extern uint32_t timer_tick;


void timer_init();


#endif
