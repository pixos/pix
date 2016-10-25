/*_
 * Copyright (c) 2015-2016 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last)      __builtin_va_start((ap), (last))
#define va_arg                  __builtin_va_arg
#define va_end(ap)              __builtin_va_end(ap)
#define va_copy(dest, src)      __builtin_va_copy((dest), (src))
#define alloca(size)            __builtin_alloca((size))

#define STRFMT_MOD_NONE         0
#define STRFMT_MOD_LONG         1
#define STRFMT_MOD_LONGLONG     2

struct strfmt_format {
    /* Leading suffix */
    int zero;
    /* Minimum length */
    int pad;
    /* Precision */
    int prec;
    /* Modifier */
    int mod;
    /* Specifier */
    int spec;
};

static off_t _output_percent(char *, size_t);
static off_t _output_char(char *, size_t, struct strfmt_format *, va_list);
static off_t _output_decimal(char *, size_t, struct strfmt_format *, va_list);
static off_t
_output_hexdecimal(char *, size_t, struct strfmt_format *, va_list, int);
static off_t _output_pointer(char *, size_t, struct strfmt_format *, va_list);
static off_t _output_string(char *, size_t, struct strfmt_format *, va_list);

/*
 * Parse the format chunk
 */
static int
_parse_format(const char *__restrict__*formatp, struct strfmt_format *strfmt)
{
    const char *format;
    /* Leading suffix */
    int zero;
    /* Minimum length */
    int pad;
    /* Precision */
    int prec;
    /* Modifier */
    int mod;
    /* Specifier */
    int spec;

    /* Reset */
    zero = 0;
    pad = 0;
    prec = 0;
    mod = 0;
    spec = 0;

    /* Take the format string */
    format = *formatp;

    /* Check and skip % */
    if ( '%' != *format ) {
        return -1;
    }
    format++;

    /* Padding with zero? */
    if ( '0' == *format ) {
        zero = 1;
        format++;
    }

    /* Padding length */
    if ( *format >= '1' && *format <= '9' ) {
        pad += *format - '0';
        format++;
        while ( *format >= '0' && *format <= '9' ) {
            pad *= 10;
            pad += *format - '0';
            format++;
        }
    }

    /* Precision */
    if ( '.' == *format ) {
        format++;
        while ( *format >= '0' && *format <= '9' ) {
            prec *= 10;
            prec += *format - '0';
            format++;
        }
    }

    /* Modifier */
    if ( 'l' == *format ) {
        format++;
        if ( 'l' == *format ) {
            mod = STRFMT_MOD_LONGLONG;
            format++;
        } else {
            mod = STRFMT_MOD_LONG;
        }
    }

    /* Conversion */
    if ( '\0' != *format ) {
        spec = *format;
        format++;
    } else {
        spec = 0;
    }

    /* Write back the current pointer of the string */
    *formatp = format;

    /* Set the parsed result */
    strfmt->zero = zero;
    strfmt->pad = pad;
    strfmt->prec = prec;
    strfmt->mod = mod;
    strfmt->spec = spec;

    return 0;
}

/*
 * Output
 */
static off_t
_output(char *str, size_t size, struct strfmt_format *strfmt, va_list ap)
{
    off_t ret;

    ret = 0;
    switch ( strfmt->spec ) {
    case '%':
        /* % */
        ret += _output_percent(str, size);
        break;
    case 's':
        /* String */
        ret += _output_string(str, size, strfmt, ap);
        break;
    case 'c':
        /* Character */
        ret += _output_char(str, size, strfmt, ap);
        break;
    case 'd':
        /* Decimal */
        ret += _output_decimal(str, size, strfmt, ap);
        break;
    case 'x':
        /* Hexdecimal (lower case) */
        ret += _output_hexdecimal(str, size, strfmt, ap, 0);
        break;
    case 'X':
        /* Hexdecimal (upper case) */
        ret += _output_hexdecimal(str, size, strfmt, ap, 1);
        break;
    case 'p':
        /* Pointer */
        ret += _output_pointer(str, size, strfmt, ap);
        break;
    default:
        /* To implement */
        ;
    }

    return ret;
}
static off_t
_output_percent(char *str, size_t size)
{
    if ( NULL != str && size > 1 ) {
        str[0] = '%';
    }
    return 1;
}
static off_t
_output_char(char *str, size_t size, struct strfmt_format *strfmt, va_list ap)
{
    char val;

    /* Get the value from an argument */
    val = (char)va_arg(ap, int);

    if ( NULL != str && size > 1 ) {
        str[0] = val;
    }

    return 1;
}
static off_t
_output_decimal(char *str, size_t size, struct strfmt_format *strfmt,
                va_list ap)
{
    long long int val;
    long long int q;
    long long int r;
    off_t pos;
    off_t ptr;
    int sz;
    int i;
    char *buf;
    int sign;

    /* Get the value from an argument */
    switch ( strfmt->mod ) {
    case STRFMT_MOD_LONG:
        val = (long int)va_arg(ap, long int);
        break;
    case STRFMT_MOD_LONGLONG:
        val = (long long int)va_arg(ap, long long int);
        break;
    default:
        val = (int)va_arg(ap, int);
    }

    /* Calculate the maximum buffer size, and allocate a buffer to store a
       decimal string */
    sz = 3 * sizeof(val) + 1; /* max. 3 chars per byte (rough approx.) + sign */
    buf = alloca(sz);
    ptr = 0;

    /* Signed? */
    if ( val < 0 ) {
        sign = -1;
        val *= -1;
    } else {
        sign = 1;
    }

    /* Store the decimal string to the buffer */
    q = val;
    while ( q ) {
        r = q % 10;
        q = q / 10;
        buf[ptr] = r + '0';
        ptr++;
    }
    if ( !ptr ) {
        buf[ptr] = '0';
        ptr++;
    }

    /* Append a sign mark */
    if ( sign < 0 ) {
        buf[ptr] = '-';
        ptr++;
    }

    /* Output the formatted string */
    pos = 0;

    /* Padding */
    if ( strfmt->pad > strfmt->prec && strfmt->pad > ptr ) {
        for ( i = 0; i < strfmt->pad - strfmt->prec && i < strfmt->pad - ptr;
              i++ ) {
            if ( (size_t)pos + 1 < size ) {
                if ( strfmt->zero ) {
                    str[pos] = '0';
                } else {
                    str[pos] = ' ';
                }
            }
            pos++;
        }
    }

    /* Precision */
    if ( strfmt->prec > ptr ) {
        for ( i = 0; i < strfmt->prec - ptr; i++ ) {
            if ( (size_t)pos + 1 < size ) {
                str[pos] = '0';
            }
            pos++;
        }
    }

    /* Value */
    for ( i = 0; i < ptr; i++ ) {
        if ( (size_t)pos + 1 < size ) {
            str[pos] = buf[ptr - i - 1];
        }
        pos++;
    }

    return pos;
}
static off_t
_output_hexdecimal(char *str, size_t size, struct strfmt_format *strfmt,
                   va_list ap, int cap)
{
    unsigned long long int val;
    unsigned long long int q;
    unsigned long long int r;
    off_t pos;
    off_t ptr;
    int sz;
    int i;
    char *buf;

    /* Get the value from an argument */
    switch ( strfmt->mod ) {
    case STRFMT_MOD_LONG:
        val = (unsigned long int)va_arg(ap, unsigned long int);
        break;
    case STRFMT_MOD_LONGLONG:
        val = (unsigned long long int)va_arg(ap, unsigned long long int);
        break;
    default:
        val = (unsigned int)va_arg(ap, unsigned int);
    }

    /* Calculate the maximum buffer size, and allocate a buffer to store a
       decimal string */
    sz = 2 * sizeof(val);       /* max. 3 chars per byte (rough approx.) */
    buf = alloca(sz);
    ptr = 0;

    /* Store the decimal string to the buffer */
    q = val;
    while ( q ) {
        r = q & 0xf;
        q = q >> 4;
        if ( r < 10 ) {
            buf[ptr] = r + '0';
        } else if ( cap ) {
            buf[ptr] = r + 'A' - 10;
        } else {
            buf[ptr] = r + 'a' - 10;
        }
        ptr++;
    }
    if ( !ptr ) {
        buf[ptr] = '0';
        ptr++;
    }

    /* Output the formatted string */
    pos = 0;

    /* Padding */
    if ( strfmt->pad > strfmt->prec && strfmt->pad > ptr ) {
        for ( i = 0; i < strfmt->pad - strfmt->prec && i < strfmt->pad - ptr;
              i++ ) {
            if ( (size_t)pos + 1 < size ) {
                if ( strfmt->zero ) {
                    str[pos] = '0';
                } else {
                    str[pos] = ' ';
                }
            }
            pos++;
        }
    }

    /* Precision */
    if ( strfmt->prec > ptr ) {
        for ( i = 0; i < strfmt->prec - ptr; i++ ) {
            if ( (size_t)pos + 1 < size ) {
                str[pos] = '0';
            }
            pos++;
        }
    }

    /* Value */
    for ( i = 0; i < ptr; i++ ) {
        if ( (size_t)pos + 1 < size ) {
            str[pos] = buf[ptr - i - 1];
        }
        pos++;
    }

    return pos;
}
static off_t
_output_pointer(char *str, size_t size, struct strfmt_format *strfmt,
                va_list ap)
{
    unsigned long long int val;
    unsigned long long int q;
    unsigned long long int r;
    off_t pos;
    off_t ptr;
    int sz;
    int i;
    char *buf;

    /* Get the value from an argument */
    val = (unsigned long long int)va_arg(ap, unsigned long long int);

    /* Calculate the maximum buffer size, and allocate a buffer to store a
       decimal string */
    sz = 2 * sizeof(val);       /* max. 3 chars per byte (rough approx.) */
    buf = alloca(sz);
    ptr = 0;

    /* Store the decimal string to the buffer */
    q = val;
    while ( q ) {
        r = q & 0xf;
        q = q >> 4;
        if ( r < 10 ) {
            buf[ptr] = r + '0';
        } else {
            buf[ptr] = r + 'a' - 10;
        }
        ptr++;
    }
    if ( !ptr ) {
        buf[ptr] = '0';
        ptr++;
    }

    /* Output the formatted string */
    pos = 0;

    /* Padding */
    if ( strfmt->pad > strfmt->prec && strfmt->pad > ptr ) {
        for ( i = 0; i < strfmt->pad - strfmt->prec && i < strfmt->pad - ptr;
              i++ ) {
            if ( (size_t)pos + 1 < size ) {
                if ( strfmt->zero ) {
                    str[pos] = '0';
                } else {
                    str[pos] = ' ';
                }
            }
            pos++;
        }
    }

    /* Precision */
    if ( strfmt->prec > ptr ) {
        for ( i = 0; i < strfmt->prec - ptr; i++ ) {
            if ( (size_t)pos + 1 < size ) {
                str[pos] = '0';
            }
            pos++;
        }
    }

    /* Value */
    for ( i = 0; i < ptr; i++ ) {
        if ( (size_t)pos + 1 < size ) {
            str[pos] = buf[ptr - i - 1];
        }
        pos++;
    }

    return pos;
}
static off_t
_output_string(char *str, size_t size, struct strfmt_format *strfmt, va_list ap)
{
    char *s;
    off_t pos;

    /* Get the value */
    s = va_arg(ap, char *);
    if ( NULL == s ) {
        s = "(null)";
    }

    pos = 0;
    while ( s[pos] ) {
        if ( (size_t)pos + 1 < size ) {
            str[pos] = s[pos];
        }
        pos++;
    }

    return pos;
}

/*
 * vsnprintf -- formated output conversion
 *
 * RETURN VALUES
 *      If successful, the vsnprintf() function returns the number of
 *      characters that would have been printed if the size were unlimited (not
 *      including the trailing `\0' used to end output to strings).  It returns
 *      the value of -1 if an error occurs.
 */
int
vsnprintf(char *__restrict__ str, size_t size,
          const char *__restrict__ format, va_list ap)
{
    struct strfmt_format strfmt;
    off_t pos;

    /* Look through the format string until its end */
    pos = 0;
    while ( '\0' != *format ) {
        if ( '%' == *format ) {
            /* % character  */
            _parse_format(&format, &strfmt);
            if ( (off_t)size > pos ) {
                pos += _output(str + pos, size - pos, &strfmt, ap);
            } else {
                pos += _output(NULL, 0, &strfmt, ap);
            }
        } else {
            /* Ordinary character */
            if ( (size_t)pos + 1 < size ) {
                str[pos] = *format;
            }
            format++;
            pos++;
        }
    }

    /* Insert a trailing '\0' */
    if ( (size_t)pos + 1 < size ) {
        str[pos] = '\0';
    }
    str[size - 1] = '\0';

    return (int)pos;
}

/*
 * snprintf
 */
int
snprintf(char *__restrict__ str, size_t size, const char *__restrict__ format,
         ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);

    return ret;
}

/*
 * fprintf
 */
int
fprintf(FILE * __restrict__ stream, const char * __restrict__ format, ...)
{
    char buf[4096];
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vsnprintf(buf, 4096, format, ap);
    va_end(ap);

    /* FIXME: Need to support more buffer */
    fputs(buf, stream);

    return ret;
}

/*
 * printf
 */
int
printf(const char * __restrict__ format, ...)
{
    char buf[4096];
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vsnprintf(buf, 4096, format, ap);
    va_end(ap);

    /* FIXME: Need to support more buffer */
    fputs(buf, stdout);

    return ret;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
