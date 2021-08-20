/**
 * Common formatted printing utilities.
 *
 * Format specifier := %[special][width][.precision][length]<type>. Only
 * limited features are provided (documented in the wiki page).
 */


#include <stdarg.h>
#include <stdbool.h>
#include <float.h>      /** For DBL_MAX. */

#include "printf.h"
#include "types.h"
#include "string.h"

#include "../common/spinlock.h"

#include "../display/terminal.h"


/**
 * Turn off lock acquiring for terminal printing on assertion failures,
 * because assertions are used in `cli_pop()` itself.
 */
bool printf_to_hold_lock = true;


/** Internal format specifier flags. */
#define FLAG_ZEROPAD   (1 << 0)
#define FLAG_LEFTALIGN (1 << 1)
#define FLAG_PLUSSIGN  (1 << 2)
#define FLAG_SPACESIGN (1 << 3)
#define FLAG_PREFIX    (1 << 4)
#define FLAG_LEN_LONG  (1 << 5)
#define FLAG_LEN_SHORT (1 << 6)
#define FLAG_LEN_CHAR  (1 << 7)
#define FLAG_WIDTH     (1 << 8)
#define FLAG_PRECISION (1 << 9)
#define FLAG_UPPERCASE (1 << 10)


/** Internal num -> string conversion buffer size. 64 is normally enough. */
#define INTERNAL_BUF_SIZE 64


/** When not specified, a float prints 6 digits after the decimal point. */
#define DEFAULT_FLOAT_PRECISION 6


/** Largest float abs value suitable for '%f' is 1e9. */
#define MAX_FLOAT_VAL 1e9


/**
 * Internal *decimal* string -> unsigned int conversion function.
 * Accepts a pointer to a (char *) pointer.
 * Side effect: (*ptr) will be modified to point to the next character right
 *              after all the consecutive valid chars.
 */
static unsigned int
_stou(const char **ptr)
{
    unsigned int num = 0;
    while (isdigit(**ptr)) {
        num = num * 10 + (unsigned int) ((**ptr) - '0');
        (*ptr)++;
    }
    return num;
}

/**
 * Reverse a buffered string BUF and handle space padding. Assumes that BUF
 * has sufficient memory and the current string w/o padding ends at LEN.
 * BUF will be filled with the padded reversed string.
 * Returns length of the final string.
 */
static size_t
_reverse_and_pad(char *buf, size_t len, unsigned int width, unsigned int flags)
{
    char temp[len];
    for (size_t i = 0; i < len; ++i)
        temp[i] = buf[i];   /** TEMP is a direct copy. */

    size_t idx = 0;
    if (!(flags & FLAG_LEFTALIGN) && !(flags & FLAG_ZEROPAD) && (width > 0)) {
        while ((idx + len < width) && (idx < INTERNAL_BUF_SIZE))
            buf[idx++] = ' ';
    }

    while (len > 0)
        buf[idx++] = temp[--len];   /** Reverse happens here. */

    if ((flags & FLAG_LEFTALIGN) && (width > 0)) {
        while ((idx < width) && (idx < INTERNAL_BUF_SIZE))
            buf[idx++] = ' ';
    }

    return idx;
}

/**
 * Internal integer -> string conversion functions.
 * Accepts a pointer to an internal buffer whose length is at least 32, the
 *         number's abs value, its sign, and other format parameters.
 * Returns length of the final string.
 */
static size_t
_itos(char *buf, unsigned long val, bool negative, unsigned long base,
      unsigned int width, unsigned int flags)
{
    size_t len = 0;

    /** Convert VAL to *reverse* BASE-ary string representation. */
    do {
        const char digit = (char) (val % base);
        buf[len++] = digit < 10 ? '0' + digit
                                : (flags & FLAG_UPPERCASE ? 'A' : 'a')
                                  + digit - 10;
        val /= base;
    } while ((val > 0) && (len < INTERNAL_BUF_SIZE));

    /** Pad leading zeros. */
    if (!(flags & FLAG_LEFTALIGN) && (flags & FLAG_ZEROPAD)) {
        if ((width > 0) && (negative | (flags & FLAG_PLUSSIGN)
                                     | (flags & FLAG_SPACESIGN)))
            width--;
        while ((len < width) && (len < INTERNAL_BUF_SIZE))
            buf[len++] = '0';
    }

    /** Add prefix. */
    if (flags & FLAG_PREFIX) {
        if ((len > 1) && (len == width)     /** If zero-padded. */
            && buf[len - 1] == '0' && buf[len - 2] == '0')
            len -= 2;
        if (len + 1 < INTERNAL_BUF_SIZE) {
            switch (base) {
            case  2: buf[len++] = 'b'; break;
            case  8: buf[len++] = 'o'; break;
            case 16:
                if (flags & FLAG_UPPERCASE)
                    buf[len++] = 'X';
                else
                    buf[len++] = 'x';
                break;
            }
            buf[len++] = '0';
        }
    }

    /** Add +/- sign. */
    if (len < INTERNAL_BUF_SIZE) {
        if (negative)
            buf[len++] = '-';
        else if (flags & FLAG_PLUSSIGN)
            buf[len++] = '+';
        else if (flags & FLAG_SPACESIGN)
            buf[len++] = ' ';   /** Ignored when '+' given. */
    }

    /** Reverse the buffered string while padding spaces. */
    return _reverse_and_pad(buf, len, width, flags);
}

/**
 * Internal float -> string conversion functions.
 * Accepts a pointer to an internal buffer whose length is at least 32, the
 *         number's abs value, its sign, and other format parameters.
 * Returns length of the final string.
 */
static size_t
_ftos(char *buf, double val, bool negative,
      unsigned int width, unsigned int precision, unsigned int flags)
{
    size_t len = 0;

    /** Test for special values. */
    if (flags & FLAG_UPPERCASE) {
        if (val != val) {                           /** "NAN". */
            strncpy(buf, "NAN", 3);
            return _reverse_and_pad(buf, 3, width, flags);
        } else if ((val > DBL_MAX) && negative) {   /** "-INF". */
            strncpy(buf, "FNI-", 4);
            return _reverse_and_pad(buf, 4, width, flags);
        } else if ((val > DBL_MAX) && !negative) {  /** "+INF". */
            if (flags & FLAG_PLUSSIGN) {
                strncpy(buf, "FNI+", 4);
                return _reverse_and_pad(buf, 4, width, flags);
            } else {
                strncpy(buf, "FNI", 3);
                return _reverse_and_pad(buf, 3, width, flags);
            }
        }
    } else {
        if (val != val) {                           /** "nan". */
            strncpy(buf, "nan", 3);
            return _reverse_and_pad(buf, 3, width, flags);
        } else if ((val > DBL_MAX) && negative) {   /** "-inf". */
            strncpy(buf, "fni-", 4);
            return _reverse_and_pad(buf, 4, width, flags);
        } else if ((val > DBL_MAX) && !negative) {  /** "+inf". */
            if (flags & FLAG_PLUSSIGN) {
                strncpy(buf, "fni+", 4);
                return _reverse_and_pad(buf, 4, width, flags);
            } else {
                strncpy(buf, "fni", 3);
                return _reverse_and_pad(buf, 3, width, flags);
            }
        }
    }

    /** Reject overlarge values. */
    if ((val > MAX_FLOAT_VAL) || (val < -MAX_FLOAT_VAL))
        return 0;

    /** Set default precision and limit precision to <= 9. */
    if (!(flags & FLAG_PRECISION))
        precision = DEFAULT_FLOAT_PRECISION;
    if (precision > 9)
        precision = 9;

    /** Split the number into whole part and fraction part. */
    const double pow10[] = {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
    };
    unsigned long whole = (unsigned long) val;
    double frac_f = (val - whole) * pow10[precision];
    unsigned long frac_i = (unsigned long) frac_f;

    /** Handle round up. */
    if (frac_f - frac_i >= 0.5) {
        frac_i++;
        if (frac_i >= pow10[precision]) {   /** Rollover. */
            frac_i = 0;
            whole++;
        }
    }

    /** Convert FRAC_I to *reverse* string representation. */
    do {
        buf[len++] = '0' + (char) (frac_i % 10);
        frac_i /= 10;
    } while ((frac_i > 0) && (len < INTERNAL_BUF_SIZE - 1));

    while ((len < precision) && (len < INTERNAL_BUF_SIZE))
        buf[len++] = '0';   /** Extra fraction part zeros. */

    if (len < INTERNAL_BUF_SIZE)
        buf[len++] = '.';   /** Decimal point. */

    /** Convert WHOLE to *reverse* string representation. */
    do {
        buf[len++] = '0' + (char) (whole % 10);
        whole /= 10;
    } while ((whole > 0) && (len < INTERNAL_BUF_SIZE));

    /** Pad leading zeros. */
    if (!(flags & FLAG_LEFTALIGN) && (flags & FLAG_ZEROPAD)) {
        if ((width > 0) && (negative | (flags & FLAG_PLUSSIGN)
                                     | (flags & FLAG_SPACESIGN)))
            width--;
        while ((len < width) && (len < INTERNAL_BUF_SIZE))
            buf[len++] = '0';
    }

    /** Add +/- sign. */
    if (len < INTERNAL_BUF_SIZE) {
        if (negative)
            buf[len++] = '-';
        else if (flags & FLAG_PLUSSIGN)
            buf[len++] = '+';
        else if (flags & FLAG_SPACESIGN)
            buf[len++] = ' ';   /** Ignored when '+' given. */
    }

    /** Reverse the buffered string while padding spaces. */
    return _reverse_and_pad(buf, len, width, flags);
}


/**
 * Main part of the formatted printing implementation to terminal.
 */
static void
_vprintf(vga_color_t fg, const char *fmt, va_list va)
{
    /**
     * A segment is a subarray between formatted specifiers that should be
     * output without modifications.
     * Pointer comparision & subtraction should work correctly.
     */
    const char *seg_start = fmt;
    const char *seg_end   = fmt;

    while (true) {

        /**
         * String end. Write the buffered segment (if there is one), then end
         * the loop.
         */
        if (*fmt == '\0') {
            if (seg_end > seg_start) {
                terminal_write_color(seg_start,
                                     (size_t) (seg_end - seg_start), fg);
            }
            break;
        }

        /**
         * Format specifier. Write the buffered segment (if there is one),
         * then start processing the format specifier.
         */
        if (*fmt == '%') {
            if (seg_end > seg_start) {
                terminal_write_color(seg_start,
                                     (size_t) (seg_end - seg_start), fg);
            }
            seg_start = fmt;
            seg_end   = fmt;

            fmt++;
            unsigned int flags = 0;

            /** Evaluate special flags. */
            bool more_flags = true;
            do {
                switch (*fmt) {
                case '0': flags |= FLAG_ZEROPAD;   fmt++; break;
                case '-': flags |= FLAG_LEFTALIGN; fmt++; break;
                case '+': flags |= FLAG_PLUSSIGN;  fmt++; break;
                case ' ': flags |= FLAG_SPACESIGN; fmt++; break;
                case '#': flags |= FLAG_PREFIX;    fmt++; break;
                default:  more_flags = false;             break;
                }
            } while (more_flags);

            /** Evaluate width field. */
            unsigned int width = 0;
            if (isdigit(*fmt)) {
                flags |= FLAG_WIDTH;
                width = _stou(&fmt);
            }

            /** Evaluate precision field. */
            unsigned int precision = 0;
            if (*fmt == '.') {
                flags |= FLAG_PRECISION;
                fmt++;
                if (isdigit(*fmt))
                    precision = _stou(&fmt);
            }

            /** Evaluate length field. Assumes 'size_t' is of length long. */
            switch (*fmt) {
            case 'l': flags |= FLAG_LEN_LONG;  fmt++; break;
            case 'h': flags |= FLAG_LEN_SHORT; fmt++; break;
            case 'r': flags |= FLAG_LEN_CHAR;  fmt++; break;
            case 'z': flags |= FLAG_LEN_LONG;  fmt++; break;
            default:                                  break;
            }

            /** Switch on the type specifier. */
            bool valid_specifier = true;
            switch (*fmt) {

            case 'd':   /** Integer types. */
            case 'i':
            case 'u':
            case 'b':
            case 'o':
            case 'X':
            case 'x': ;
                unsigned long base = 10;

                /** Set the base for different aries. */
                switch (*fmt) {
                case 'b': base = 2;  break;
                case 'o': base = 8;  break;
                case 'X':
                case 'x': base = 16; break;
                case 'u':
                case 'd':
                case 'i': base = 10; break;
                }

                if (base == 10)     /** No prefix for decimals. */
                    flags &= ~FLAG_PREFIX;

                if ((*fmt != 'i') && (*fmt != 'd'))     /** Limit signs. */
                    flags &= ~(FLAG_PLUSSIGN | FLAG_SPACESIGN);

                if (*fmt == 'X')    /** Set uppercase for 'X'. */
                    flags |= FLAG_UPPERCASE;

                /**
                 * Read in the corresponding argument. Note how we set the
                 * second argument of 'va_arg' here. Check the first section
                 * of https://www.eskimo.com/~scs/cclass/int/sx11c.html :)
                 */
                unsigned long i_val;
                bool i_negative;
                if (*fmt == 'i' || *fmt == 'd') {   /** Signed. */
                    long raw_val;
                    if (flags & FLAG_LEN_LONG)
                        raw_val = va_arg(va, long);
                    else
                        raw_val = va_arg(va, int);
                    i_negative = raw_val < 0;
                    i_val = (unsigned long) i_negative ? 0 - raw_val : raw_val;
                } else {                            /** Unsigned. */
                    if (flags & FLAG_LEN_LONG)
                        i_val = va_arg(va, unsigned long);
                    else
                        i_val = va_arg(va, unsigned int);
                    i_negative = false;
                }

                /** Format the number into a string in buffer. */
                char i_buf[INTERNAL_BUF_SIZE];
                size_t i_written = _itos(i_buf, i_val, i_negative,
                                         base, width, flags);

                /** Dump the internal buffer to actual output. */
                terminal_write_color(i_buf, i_written, fg);

                fmt++;
                break;

            case 'f':   /** Float types. */
            case 'F':

                if (*fmt == 'F')    /** Set uppercase for 'F'. */
                    flags |= FLAG_UPPERCASE;

                /**
                 * Read in the corresponding argument. Note how we set the
                 * second argument of 'va_arg' here. Check the first section
                 * of https://www.eskimo.com/~scs/cclass/int/sx11c.html ;)
                 */
                double f_val = va_arg(va, double);
                bool f_negative = f_val < 0;
                if (f_negative)
                    f_val = 0 - f_val;

                /** Format the number into a string in buffer. */
                char f_buf[INTERNAL_BUF_SIZE];
                size_t f_written = _ftos(f_buf, f_val, f_negative,
                                         width, precision, flags);

                /** Dump the internal buffer to actual output. */
                terminal_write_color(f_buf, f_written, fg);

                fmt++;
                break;

            case 'c': ; /** Single char. */

                char c_val = (char) va_arg(va, int);

                if (width > 1) {
                    char c_buf[width - 1];
                    for (size_t i = 0; i < width - 1; ++i)
                        c_buf[i] = ' ';

                    if (flags & FLAG_LEFTALIGN) {
                        terminal_write_color(&c_val, 1, fg);
                        terminal_write_color(c_buf, width - 1, fg);
                    } else {
                        terminal_write_color(c_buf, width - 1, fg);
                        terminal_write_color(&c_val, 1, fg);
                    }
                } else
                    terminal_write_color(&c_val, 1, fg);

                fmt++;
                break;

            case 's': ; /** String. */
                
                const char *s_ptr = va_arg(va, char *);
                size_t s_len = strlen(s_ptr);

                if (width > s_len) {
                    char s_buf[width - s_len];
                    for (size_t i = 0; i < width - s_len; ++i)
                        s_buf[i] = ' ';

                    if (flags & FLAG_LEFTALIGN) {
                        terminal_write_color(s_ptr, s_len, fg);
                        terminal_write_color(s_buf, width - s_len, fg);
                    } else {
                        terminal_write_color(s_ptr, s_len, fg);
                        terminal_write_color(s_buf, width - s_len, fg);
                    }
                } else
                    terminal_write_color(s_ptr, s_len, fg);

                fmt++;
                break;

            case 'p':   /** Pointer type. */

                /** Pointer type := %0X padding to 2x length of (void *). */
                width = 2 * sizeof(void *);
                flags |= FLAG_UPPERCASE | FLAG_ZEROPAD;

                unsigned long p_val = (unsigned long) va_arg(va, void *);

                char p_buf[INTERNAL_BUF_SIZE];
                size_t p_written = _itos(p_buf, p_val, false,
                                         16, width, flags);

                terminal_write_color(p_buf, p_written, fg);

                fmt++;
                break;

            case '%':   /** '%%' interpreted as a single '%'. */

                terminal_write_color("%", 1, fg);
                
                fmt++;
                break;

            default: valid_specifier = false; break;
            }

            /** Invalid specifier treated as normal chars in FMT string. */
            if (valid_specifier)
                seg_start = fmt;
            seg_end = fmt;

            continue;
        }

        /** Normal characters are simply buffered. */
        seg_end++;
        fmt++;
    }
}

/**
 * Main part of the formatted printing implementation to string buffer.
 */
static void
_vsnprintf(char *buf, size_t count, const char *fmt, va_list va)
{
    size_t remain = count;  /** Remaining available number of chars. */

    /**
     * A segment is a subarray between formatted specifiers that should be
     * output without modifications.
     * Pointer comparision & subtraction should work correctly.
     */
    const char *seg_start = fmt;
    const char *seg_end   = fmt;

    while (true) {

        /**
         * String end. Write the buffered segment (if there is one), then end
         * the loop.
         */
        if (*fmt == '\0') {
            if (seg_end > seg_start) {
                size_t size = (size_t) (seg_end - seg_start);
                if (remain < size)
                    size = remain;
                memcpy(buf, seg_start, size);
                buf += size;
                remain -= size;
                if (remain == 0) {
                    *buf = '\0';
                    return;
                }
            }
            break;
        }

        /**
         * Format specifier. Write the buffered segment (if there is one),
         * then start processing the format specifier.
         */
        if (*fmt == '%') {
            if (seg_end > seg_start) {
                size_t size = (size_t) (seg_end - seg_start);
                if (remain < size)
                    size = remain;
                memcpy(buf, seg_start, size);
                buf += size;
                remain -= size;
                if (remain == 0) {
                    *buf = '\0';
                    return;
                }
            }
            seg_start = fmt;
            seg_end   = fmt;

            fmt++;
            unsigned int flags = 0;

            /** Evaluate special flags. */
            bool more_flags = true;
            do {
                switch (*fmt) {
                case '0': flags |= FLAG_ZEROPAD;   fmt++; break;
                case '-': flags |= FLAG_LEFTALIGN; fmt++; break;
                case '+': flags |= FLAG_PLUSSIGN;  fmt++; break;
                case ' ': flags |= FLAG_SPACESIGN; fmt++; break;
                case '#': flags |= FLAG_PREFIX;    fmt++; break;
                default:  more_flags = false;             break;
                }
            } while (more_flags);

            /** Evaluate width field. */
            unsigned int width = 0;
            if (isdigit(*fmt)) {
                flags |= FLAG_WIDTH;
                width = _stou(&fmt);
            }

            /** Evaluate precision field. */
            unsigned int precision = 0;
            if (*fmt == '.') {
                flags |= FLAG_PRECISION;
                fmt++;
                if (isdigit(*fmt))
                    precision = _stou(&fmt);
            }

            /** Evaluate length field. Assumes 'size_t' is of length long. */
            switch (*fmt) {
            case 'l': flags |= FLAG_LEN_LONG;  fmt++; break;
            case 'h': flags |= FLAG_LEN_SHORT; fmt++; break;
            case 'r': flags |= FLAG_LEN_CHAR;  fmt++; break;
            case 'z': flags |= FLAG_LEN_LONG;  fmt++; break;
            default:                                  break;
            }

            /** Switch on the type specifier. */
            bool valid_specifier = true;
            switch (*fmt) {

            case 'd':   /** Integer types. */
            case 'i':
            case 'u':
            case 'b':
            case 'o':
            case 'X':
            case 'x': ;
                unsigned long base = 10;

                /** Set the base for different aries. */
                switch (*fmt) {
                case 'b': base = 2;  break;
                case 'o': base = 8;  break;
                case 'X':
                case 'x': base = 16; break;
                case 'u':
                case 'd':
                case 'i': base = 10; break;
                }

                if (base == 10)     /** No prefix for decimals. */
                    flags &= ~FLAG_PREFIX;

                if ((*fmt != 'i') && (*fmt != 'd'))     /** Limit signs. */
                    flags &= ~(FLAG_PLUSSIGN | FLAG_SPACESIGN);

                if (*fmt == 'X')    /** Set uppercase for 'X'. */
                    flags |= FLAG_UPPERCASE;

                /**
                 * Read in the corresponding argument. Note how we set the
                 * second argument of 'va_arg' here. Check the first section
                 * of https://www.eskimo.com/~scs/cclass/int/sx11c.html :)
                 */
                unsigned long i_val;
                bool i_negative;
                if (*fmt == 'i' || *fmt == 'd') {   /** Signed. */
                    long raw_val;
                    if (flags & FLAG_LEN_LONG)
                        raw_val = va_arg(va, long);
                    else
                        raw_val = va_arg(va, int);
                    i_negative = raw_val < 0;
                    i_val = (unsigned long) i_negative ? 0 - raw_val : raw_val;
                } else {                            /** Unsigned. */
                    if (flags & FLAG_LEN_LONG)
                        i_val = va_arg(va, unsigned long);
                    else
                        i_val = va_arg(va, unsigned int);
                    i_negative = false;
                }

                /** Format the number into a string in buffer. */
                char i_buf[INTERNAL_BUF_SIZE];
                size_t i_written = _itos(i_buf, i_val, i_negative,
                                         base, width, flags);

                /** Dump the internal buffer to actual output. */
                size_t i_size = remain < i_written ? remain : i_written;
                memcpy(buf, i_buf, i_size);
                buf += i_size;
                remain -= i_size;
                if (remain == 0) {
                    *buf = '\0';
                    return;
                }

                fmt++;
                break;

            case 'f':   /** Float types. */
            case 'F':

                if (*fmt == 'F')    /** Set uppercase for 'F'. */
                    flags |= FLAG_UPPERCASE;

                /**
                 * Read in the corresponding argument. Note how we set the
                 * second argument of 'va_arg' here. Check the first section
                 * of https://www.eskimo.com/~scs/cclass/int/sx11c.html ;)
                 */
                double f_val = va_arg(va, double);
                bool f_negative = f_val < 0;
                if (f_negative)
                    f_val = 0 - f_val;

                /** Format the number into a string in buffer. */
                char f_buf[INTERNAL_BUF_SIZE];
                size_t f_written = _ftos(f_buf, f_val, f_negative,
                                         width, precision, flags);

                /** Dump the internal buffer to actual output. */
                size_t f_size = remain < f_written ? remain : f_written;
                memcpy(buf, f_buf, f_size);
                buf += f_size;
                remain -= f_size;
                if (remain == 0) {
                    *buf = '\0';
                    return;
                }

                fmt++;
                break;

            case 'c': ; /** Single char. */

                char c_val = (char) va_arg(va, int);

                if (width > 1) {
                    char c_buf[width - 1];
                    for (size_t i = 0; i < width - 1; ++i)
                        c_buf[i] = ' ';

                    if (flags & FLAG_LEFTALIGN) {
                        size_t c_size = remain < width ? remain : width;
                        memcpy(buf, &c_val, 1);
                        memcpy(buf, c_buf, c_size - 1);
                        buf += c_size;
                        remain -= c_size;
                    } else {
                        size_t c_size = remain < width ? remain : width;
                        memcpy(buf, c_buf, c_size - 1);
                        memcpy(buf, &c_val, 1);
                        buf += c_size;
                        remain -= c_size;
                    }
                } else {
                    memcpy(buf, &c_val, 1);
                    buf++;
                    remain--;
                }

                if (remain == 0) {
                    *buf = '\0';
                    return;
                }

                fmt++;
                break;

            case 's': ; /** String. */
                
                const char *s_ptr = va_arg(va, char *);
                size_t s_len = strlen(s_ptr);

                if (width > s_len) {
                    char s_buf[width - s_len];
                    for (size_t i = 0; i < width - s_len; ++i)
                        s_buf[i] = ' ';

                    if (flags & FLAG_LEFTALIGN) {
                        size_t size1 = remain < s_len ? remain : s_len;
                        size_t size2 = remain < width ? remain : width;
                        if (size1 == remain) {
                            memcpy(buf, s_ptr, size1);
                            buf += size1;
                            remain -= size1;
                        } else {
                            memcpy(buf, s_ptr, s_len);
                            memcpy(buf, s_buf, size2 - s_len);
                            buf += size2;
                            remain -= size2;
                        }
                    } else {
                        size_t size1 = remain < s_len ? remain : s_len;
                        size_t size2 = remain < width ? remain : width;
                        if (size1 == remain) {
                            memcpy(buf, s_ptr, size1);
                            buf += size1;
                            remain -= size1;
                        } else {
                            memcpy(buf, s_buf, size2 - s_len);
                            memcpy(buf, s_ptr, s_len);
                            buf += size2;
                            remain -= size2;
                        }
                    }
                } else {
                    size_t s_size = remain < s_len ? remain : s_len;
                    memcpy(buf, s_ptr, s_size);
                    buf += s_size;
                    remain -= s_size;
                }

                if (remain == 0) {
                    *buf = '\0';
                    return;
                }

                fmt++;
                break;

            case 'p':   /** Pointer type. */

                /** Pointer type := %0X padding to 2x length of (void *). */
                width = 2 * sizeof(void *);
                flags |= FLAG_UPPERCASE | FLAG_ZEROPAD;

                unsigned long p_val = (unsigned long) va_arg(va, void *);

                char p_buf[INTERNAL_BUF_SIZE];
                size_t p_written = _itos(p_buf, p_val, false,
                                         16, width, flags);

                size_t p_size = remain < p_written ? remain : p_written;
                memcpy(buf, p_buf, p_size);
                buf += p_size;
                remain -= p_size;
                if (remain == 0) {
                    *buf = '\0';
                    return;
                }

                fmt++;
                break;

            case '%':   /** '%%' interpreted as a single '%'. */

                memcpy(buf, "%", 1);
                buf++;
                remain--;
                if (remain == 0) {
                    *buf = '\0';
                    return;
                }
                
                fmt++;
                break;

            default: valid_specifier = false; break;
            }

            /** Invalid specifier treated as normal chars in FMT string. */
            if (valid_specifier)
                seg_start = fmt;
            seg_end = fmt;

            continue;
        }

        /** Normal characters are simply buffered. */
        seg_end++;
        fmt++;
    }

    *buf = '\0';
}


/** Formatted printing to terminal window. */
void
printf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    if (printf_to_hold_lock)
        spinlock_acquire(&terminal_lock);
    _vprintf(TERMINAL_DEFAULT_COLOR_FG, fmt, va);
    if (printf_to_hold_lock)
        spinlock_release(&terminal_lock);
    
    va_end(va);
}

/** Formatted printing to terminal window with specified color FG. */
void
cprintf(vga_color_t fg, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    if (printf_to_hold_lock)
        spinlock_acquire(&terminal_lock);
    _vprintf(fg, fmt, va);
    if (printf_to_hold_lock)
        spinlock_release(&terminal_lock);
    
    va_end(va);
}

/**
 * Formatted printing to a string buffer BUF.
 * Limited to COUNT chars. Will implicitly add a trailing null byte
 * even when COUNT chars limit has been reached. (So be sure that
 * BUF is at least COUNT + 1 in length.)
 */
void
snprintf(char *buf, size_t count, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    _vsnprintf(buf, count, fmt, va);
    va_end(va);
}
