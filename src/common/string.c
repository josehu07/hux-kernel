/**
 * Common string utilities.
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

/**
 * Copies COUNT bytes from where SRC points to into where DST points to.
 * The copy is like relayed by an internal buffer, so it is OK if these
 * two memory regions overlap.
 * Returns a copy of the pointer DST.
 */
void *
memmove(void *dst, const void *src, size_t count)
{
    unsigned long int dstp = (long int) dst;
    unsigned long int srcp = (long int) src;
    if (dstp - srcp >= count)   /** Unsigned compare. */
        dst = memcpy(dst, src, count);
    else {  /** SRC region overlaps start of DST, do reversed order. */
        unsigned char *dst_copy = ((unsigned char *) dst) + count;
        unsigned char *src_copy = ((unsigned char *) src) + count;
        while (count-- > 0)
            *dst_copy-- = *src_copy--;
    }
    return dst;
}

/**
 * Compare two memory regions byte-wise. Returns zero if they are equal.
 * Returns <0 if the first unequal byte has a lower unsigned value in
 * PTR1, and >0 if higher.
 */
int
memcmp(const void *ptr1, const void *ptr2, size_t count)
{
    const char *ptr1_cast = (const char *) ptr1;
    const char *ptr2_cast = (const char *) ptr2;
    char b1 = 0, b2 = 0;
    while (count-- > 0) {
        b1 = *ptr1_cast++;
        b2 = *ptr2_cast++;
        if (b1 != b2)
            return ((int) b1) - ((int) b2);
    }
    return ((int) b1) - ((int) b2);
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
 * Length of the string (excluding the terminating '\0').
 * If string STR does not terminate before reaching COUNT chars, returns
 * COUNT.
 */
size_t
strnlen(const char *str, size_t count)
{
    size_t len = 0;
    while (str[len] && count > 0) {
        len++;
        count--;
    }
    return len;
}

/**
 * Compare two strings, returning less than, equal to or greater than zero
 * if STR1 is lexicographically less than, equal to or greater than S2.
 * Limited to upto COUNT chars.
 */
int
strncmp(const char *str1, const char *str2, size_t count)
{
    char c1 = '\0', c2 = '\0';
    while (count-- > 0) {
        c1 = *str1++;
        c2 = *str2++;
        if (c1 == '\0' || c1 != c2)
            return ((int) c1) - ((int) c2);
    }
    return ((int) c1) - ((int) c2);
}

/**
 * Copy string SRC to DST. Assume DST is large enough.
 * Limited to upto COUNT chars. Adds implicit null terminator even if
 * COUNT is smaller than actual length of SRC.
 */
char *
strncpy(char *dst, const char *src, size_t count)
{
    size_t size = strnlen(src, count);
    if (size != count)
        memset(dst + size, '\0', count - size);
    dst[size] = '\0';
    return memcpy(dst, src, size);
}

/**
 * Concatenate string DST with SRC. Assume DST is large enough.
 * Returns a copy of the pointer DST.
 * Limited to upto COUNT chars.
 */
char *
strncat(char *dst, const char *src, size_t count)
{
    char *s = dst;
    dst += strlen(dst);
    size_t size = strnlen(src, count);
    dst[size] = '\0';
    memcpy(dst, src, size);
    return s;
}
