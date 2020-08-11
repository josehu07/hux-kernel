/**
 * Common debugging utilities.
 */


#ifndef DEBUG_H
#define DEBUG_H


void debug_init();

void stack_trace();


/** Panicking macro. */
// #define 


/** Assertion macro. */
#define ASSERT_FAIL_MSG_BUF_LEN 256
#define assert(condition)                                                     \
    do {                                                                      \
        if (!(condition)) {                                                   \
            char panic_msg[ASSERT_FAIL_MSG_BUF_LEN];                          \
            sprintf(panic_msg,                                                \
                    "assertion failed @ function '%s', file '%s': line %d",   \
                    __FUNCTION__, __FILE__, __LINE__);                        \
            panic(panic_msg);                                                 \
        }                                                                     \
    } while (0)


#endif
