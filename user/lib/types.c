/**
 * Type identification functions on characters.
 */


#include "types.h"


inline bool
isdigit(char c)
{
    return ('0' <= c) && (c <= '9');
}

inline bool
isxdigit(char c)
{
    return (('0' <= c) && (c <= '9')) ||
           (('A' <= c) && (c <= 'F')) ||
           (('a' <= c) && (c <= 'f'));
}

inline bool
isupper(char c)
{
    return ('A' <= c) && (c <= 'Z');
}

inline bool
islower(char c)
{
    return ('a' <= c) && (c <= 'z');
}

inline bool
isalpha(char c)
{
    return islower(c) || isupper(c);
}

/**
 * If C is a whitespace:
 *   - space (0x20)
 *   - form feed (0x0c)
 *   - line feed (0x0a, \n)
 *   - carriage return (0x0d, \r)
 *   - horizontal tab (0x09, \t)
 *   - vertical tab (0x0b)
 */
inline bool
isspace(char c)
{
    return (c == 0x20) || (c == 0x0c) || (c == 0x0a) || (c == 0x0d) ||
           (c == 0x09) || (c == 0x0b);
}
