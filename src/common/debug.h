/**
 * Common debugging utilities.
 */


#ifndef DEBUG_H
#define DEBUG_H


#include "../boot/multiboot.h"


void debug_init(multiboot_info_t *mbi);

void stack_trace();


/** Panicking macro. */
#define panic(fmt, args...) do {                                               \
                                cprintf(VGA_COLOR_MAGENTA, "PANIC: " fmt "\n", \
                                        ##args);                               \
                                stack_trace();                                 \
                                asm volatile ( "hlt" );                        \
                            } while (0)


/** Assertion macro. */
#define assert(condition)   do {                                              \
                                if (!(condition)) {                           \
                                    panic("assertion failed @ function '%s'," \
                                          " file '%s': line %d",              \
                                          __FUNCTION__, __FILE__, __LINE__);  \
                                }                                             \
                            } while (0)


/** Error prompting macro. */
#define error(fmt, args...) do {                                           \
                                cprintf(VGA_COLOR_RED, "ERROR: " fmt "\n", \
                                        ##args);                           \
                                panic("error occurred @ function '%s',"    \
                                      " file '%s': line %d",               \
                                      __FUNCTION__, __FILE__, __LINE__);   \
                            } while (0)


#endif
