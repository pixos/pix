/*_
 * Copyright (c) 2016 Hirochika Asai <asai@jar.jp>
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
#include <unistd.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

/*
 * fgetc
 */
int
fgetc(FILE *stream)
{
    ssize_t sz;
    char c;

    sz = read(stream->fd, &c, 1);
    if ( 0 == sz ) {
        /* EOF */
        return EOF;
    } else if ( sz < 0 ) {
        /* Error: FIXME */
        return -1;
    } else {
        return c;
    }
}

/*
 * getchar
 */
int
getchar(void)
{
    return fgetc(stdin);
}

/*
 * fputc
 */
int
fputc(int c, FILE *stream)
{
    ssize_t sz;

    sz = write(stream->fd, &c, 1);
    if ( sz <= 0 ) {
        return EOF;
    } else {
        return c;
    }
}

/*
 * putchar
 */
int
putchar(int c)
{
    return fputc(c, stdout);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
