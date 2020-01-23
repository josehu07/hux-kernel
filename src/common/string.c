/**
 * Common string utilities used by many parts of the kernel.
 */


#include <stddef.h>

#include "string.h"


/**
 * Copies the byte C into the first COUNT bytes pointed to by DST.
 * Returns a copy of the pointer DST.
 */
void *
memset(void *dst, unsigned char c, size_t count)
{
    unsigned char *dst_copy = (unsigned char *) dst;
    while (count-- > 0)
        *dst_copy++ = c;
    return dst;
}

/**
 * Copies COUNT bytes from where SRC points to into where DST points to.
 * Assumes no overlapping between these two regions.
 * Returns a copy of the pointer DST.
 */
void *
memcpy(void *dst, const void *src, size_t count)
{
    unsigned char *dst_copy = (unsigned char *) dst;
    unsigned char *src_copy = (unsigned char *) src;
    while (count-- > 0)
        *dst_copy++ = *src_copy++;
    return dst;
}

/** Length of the string (excluding the terminating '\0'). */
size_t
strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

/**
 * Compare two strings, returning less than, equal to or greater than zero
 * if STR1 is lexicographically less than, equal to or greater than S2.
 */
int
strcmp(const char *str1, const char *str2)
{
    char c1, c2;
    do {
        c1 = *str1++;
        c2 = *str2++;
    } while (c1 == c2 && c1 != '\0');
    return c1 - c2;
}

/** Copy string SRC to DST. Assume DST is large enough. */
char *
strcpy(char *dst, const char *src)
{
    return memcpy(dst, src, strlen(src) + 1);
}

/**
 * Concatenate string DST with SRC. Assume DST is large enough.
 * Returns a copy of the pointer DST.
 */
char *
strcat(char *dst, const char *src)
{
    strcpy(dst + strlen(dst), src);
    return dst;
}
