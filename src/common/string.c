/**
 * Common string utilities used by many parts of the kernel.
 */


#include <stddef.h>

#include "string.h"


/** Length of the string (excluding the terminating '\0'). */
size_t
strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}
