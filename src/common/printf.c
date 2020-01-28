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

#include "../display/terminal.h"


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


/** Internal num -> string conversion buffer size. 32 is normally enough. */
#define INTERNAL_BUF_SIZE 32


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
            strcpy(buf, "NAN");
            return _reverse_and_pad(buf, 3, width, flags);
        } else if ((val > DBL_MAX) && negative) {   /** "-INF". */
            strcpy(buf, "FNI-");
            return _reverse_and_pad(buf, 4, width, flags);
        } else if ((val > DBL_MAX) && !negative) {  /** "+INF". */
            if (flags & FLAG_PLUSSIGN) {
                strcpy(buf, "FNI+");
                return _reverse_and_pad(buf, 4, width, flags);
            } else {
                strcpy(buf, "FNI");
                return _reverse_and_pad(buf, 3, width, flags);
            }
        }
    } else {
        if (val != val) {                           /** "nan". */
            strcpy(buf, "nan");
            return _reverse_and_pad(buf, 3, width, flags);
        } else if ((val > DBL_MAX) && negative) {   /** "-inf". */
            strcpy(buf, "fni-");
            return _reverse_and_pad(buf, 4, width, flags);
        } else if ((val > DBL_MAX) && !negative) {  /** "+inf". */
            if (flags & FLAG_PLUSSIGN) {
                strcpy(buf, "fni+");
                return _reverse_and_pad(buf, 4, width, flags);
            } else {
                strcpy(buf, "fni");
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
    } while ((frac_i > 0) && (len < INTERNAL_BUF_SIZE));

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
 * Output function wrapper type for 'tprintf'/'sprintf'.
 * Write SIZE bytes from SRC to (*DST), and update (*DST).
 */
typedef void (*out_func_type)(char **dst_ptr, const char *src, size_t size);

static void
out_func_terminal(char **dst_ptr, const char *src, size_t size)
{
    (void) dst_ptr;     /** Not used. */
    terminal_write(src, size);
}

static void
out_func_string(char **dst_ptr, const char *src, size_t size)
{
    memcpy(*dst_ptr, src, size);    /** Assumes memory safety in DST. */
    (*dst_ptr) += size;
}


/**
 * Main part of the formatted printing implementation.
 */
static void
_vprintf(out_func_type out_func, char **dst_ptr, const char *fmt, va_list va)
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
            if (seg_end > seg_start)
                out_func(dst_ptr, seg_start, (size_t) (seg_end - seg_start));
            break;
        }

        /**
         * Format specifier. Write the buffered segment (if there is one),
         * then start processing the format specifier.
         */
        if (*fmt == '%') {
            if (seg_end > seg_start)
                out_func(dst_ptr, seg_start, (size_t) (seg_end - seg_start));
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
                 * of https://www.eskimo.com/~scs/cclass/int/sx11c.html ;)
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
                out_func(dst_ptr, i_buf, i_written);

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
                out_func(dst_ptr, f_buf, f_written);

                fmt++;
                break;

            case 'c': ; /** Single char. */

                char c_val = (char) va_arg(va, int);

                if (width > 1) {
                    char c_buf[width - 1];
                    for (size_t i = 0; i < width - 1; ++i)
                        c_buf[i] = ' ';

                    if (flags & FLAG_LEFTALIGN) {
                        out_func(dst_ptr, &c_val, 1);
                        out_func(dst_ptr, c_buf, width - 1);
                    } else {
                        out_func(dst_ptr, c_buf, width - 1);
                        out_func(dst_ptr, &c_val, 1);
                    }
                } else
                    out_func(dst_ptr, &c_val, 1);

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
                        out_func(dst_ptr, s_ptr, s_len);
                        out_func(dst_ptr, s_buf, width - s_len);
                    } else {
                        out_func(dst_ptr, s_ptr, s_len);
                        out_func(dst_ptr, s_buf, width - s_len);
                    }
                } else
                    out_func(dst_ptr, s_ptr, s_len);

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

                out_func(dst_ptr, p_buf, p_written);

                fmt++;
                break;

            case '%':   /** '%%' interpreted as a single '%'. */

                out_func(dst_ptr, "%", 1);
                
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


/** Formatted printing to terminal window. */
void
tprintf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    _vprintf(out_func_terminal, NULL, fmt, va);

    va_end(va);
}

/** Formatted printing to a string buffer BUF. */
void
sprintf(char *buf, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    _vprintf(out_func_string, &buf, fmt, va);

    va_end(va);
}
