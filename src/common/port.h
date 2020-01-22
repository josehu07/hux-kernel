/**
 * Common I/O ports inline assembly utilities (in At&T syntax).
 */


#ifndef PORT_H
#define PORT_H


#include <stdint.h>


void outb(uint16_t port, uint8_t  val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, uint32_t val);

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);


#endif
