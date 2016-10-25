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

/*
 * Find length of string
 *
 * SYNOPSIS
 *      size_t
 *      strlen(const char *s);
 *
 * DESCRIPTION
 *      The strlen() function computes the length of the string s.
 *
 * RETURN VALUES
 *      The strlen() function returns the number of characters that precede the
 *      terminating NULL character.
 */
size_t
strlen(const char *s)
{
    size_t len;

    len = 0;
    while ( '\0' != *s ) {
        len++;
        s++;
    }

    return len;
}

/*
 * Copy strings
 *
 * SYNOPSIS
 *      char *
 *      strcpy(char *dst, const char *src);
 *
 * DESCRIPTION
 *      The strcpy() function copies the string src to dst (including the
 *      terminating '\0' character).
 *
 * RETURN VALUES
 *      The strcpy() function returns dst.
 *
 */
char *
strcpy(char *dst, const char *src)
{
    size_t i;

    i = 0;
    while ( '\0' != src[i] ) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = src[i];

    return dst;
}

/*
 * Copy strings
 *
 * SYNOPSIS
 *      char *
 *      strncpy(char *restrict dst, const char *restrict src, size_t n);
 *
 * DESCRIPTION
 *      The strncpy() function copies at most n characters from src to dst.  If
 *      src is less than n characters long, the remainder of dst is filled with
 *      `\0' characters.  Otherwise, dst is not terminated.
 *
 * RETURN VALUES
 *      The strncpy() function returns dst.
 *
 */
char *
strncpy(char *__restrict__ dst, const char *__restrict__ src, size_t n)
{
    size_t i;

    i = 0;
    while ( '\0' != src[i] && i < n ) {
        dst[i] = src[i];
        i++;
    }
    for ( ; i < n; i++ ) {
        dst[i] = '\0';
    }

    return dst;
}

/*
 * Copy strings
 *
 * SYNOPSIS
 *      size_t
 *      strlcpy(char *restrict dst, const char *restrict src, size_t n);
 *
 * DESCRIPTION
 *      The strlcpy() function copies at most n - 1 characters from src to dst.
 *      dst is not NULL-terminated.
 *
 * RETURN VALUES
 *      The strlcpy() function returns the length of the string that it tried to
 *      copy; i.e., it returns the length of src.
 *
 */
size_t
strlcpy(char *__restrict__ dst, const char *__restrict__ src, size_t n)
{
    size_t i;

    i = 0;
    while ( '\0' != src[i] && i < n - 1 ) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';

    while ( '\0' != src[i] ) {
        i++;
    }

    return i;
}

/*
 * Compare strings
 *
 * SYNOPSIS
 *      int
 *      strcmp(const char *s1, const char *s2);
 *
 * DESCRIPTION
 *      The strcmp() function lexicographically compare the null-terminated
 *      strings s1 and s2.
 *
 * RETURN VALUES
 *      The strcmp() function returns an integer greater than, equal to, or less
 *      than 0, according as the string s1 is greater than, equal to, or less
 *      than the string s2.  The comparison is done using unsigned characters,
 *      so that '\200' is greater than '\0'.
 *
 */
int
strcmp(const char *s1, const char *s2)
{
    size_t i;
    int diff;

    i = 0;
    while ( s1[i] != '\0' || s2[i] != '\0' ) {
        diff = (int)s1[i] - (int)s2[i];
        if ( diff ) {
            return diff;
        }
        i++;
    }

    return 0;
}

/*
 * Compare strings
 *
 * SYNOPSIS
 *      int
 *      strncmp(const char *s1, const char *s2, size_t n);
 *
 * DESCRIPTION
 *      The strncmp() function compares not more than n characters.
 *
 * RETURN VALUES
 *      The strncmp() function returns an integer greater than, equal to, or less
 *      than 0, according as the string s1 is greater than, equal to, or less
 *      than the string s2.  The comparison is done using unsigned characters,
 *      so that '\200' is greater than '\0'.
 *
 */
int
strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;
    int diff;

    i = 0;
    while ( (s1[i] != '\0' || s2[i] != '\0') && i < n ) {
        diff = (int)s1[i] - (int)s2[i];
        if ( diff ) {
            return diff;
        }
        i++;
    }

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
