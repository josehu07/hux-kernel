/**
 * Common I/O ports inline assembly utilities (in AT&T syntax).
 */


#include "port.h"


/** Output 8 bits to an I/O port. */
inline void
outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a" (val), "d" (port) );
}

/** Output 16 bits to an I/O port. */
inline void
outw(uint16_t port, uint16_t val)
{
    asm volatile ( "outw %0, %1" : : "a" (val), "d" (port) );
}

/** Output 32 bits to an I/O port. */
inline void
outl(uint16_t port, uint32_t val)
{
    asm volatile ( "outl %0, %1" : : "a" (val), "d" (port) );
}

/** Output CNT 32-bit dwords from the buffer at ADDR to an I/O port. */
inline void
outsl(uint16_t port, const void *addr, uint32_t cnt)
{
    asm volatile ( "rep outsl"
                   : "+S" (addr), "+c" (cnt)
                   : "d" (port) );
}


/** Input 8 bits from an I/O port. */
inline uint8_t
inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a" (ret) : "d" (port) );
    return ret;
}

/** Input 16 bits from an I/O port. */
inline uint16_t
inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a" (ret) : "d" (port) );
    return ret;
}

/** Input 32 bits from an I/O port. */
inline uint32_t
inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %1, %0" : "=a" (ret) : "d" (port) );
    return ret;
}

/** Input CNT 32-bit dwords into the buffer at ADDR. */
inline void
insl(uint16_t port, void *addr, uint32_t cnt)
{
    asm volatile ( "rep insl"
                   : "+D" (addr), "+c" (cnt)
                   : "d" (port)
                   : "memory" );
}
