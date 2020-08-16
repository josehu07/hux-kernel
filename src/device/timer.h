/**
 * Programmable interval timer (PIT) in square wave generator mode to
 * serve as the system timer.
 */


#ifndef TIMER_H
#define TIMER_H


#include <stdint.h>


void timer_init(uint16_t freq_hz);


#endif
