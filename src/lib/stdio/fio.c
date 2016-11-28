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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_BUFFER_SIZE    4096
#define FILE_MODE_READ      1
#define FILE_MODE_WRITE     2
#define FILE_MODE_APPEND    4

FILE *stdin;
FILE *stdout;
FILE *stderr;

/*
 * fdopen
 */
FILE *
fdopen(int fildes, const char *mode)
{
    FILE *f;
    int md;

    /* Check the mode */
    md = 0;
    if ( 0 == strcmp(mode, "r") || 0 == strcmp(mode, "rb") ) {
        /* For reading */
        md = FILE_MODE_READ;
    } else if ( 0 == strcmp(mode, "r+") || 0 == strcmp(mode, "rb+") ) {
        /* For reading and writing */
        md = FILE_MODE_READ | FILE_MODE_WRITE;
    } else if ( 0 == strcmp(mode, "w") || 0 == strcmp(mode, "wb") ) {
        /* For writing.  Truncate to zero or create if not exist. */
        md = FILE_MODE_WRITE;
    } else if ( 0 == strcmp(mode, "w+") || 0 == strcmp(mode, "wb+") ) {
        /* For reading and writing.  Truncate to zero or create if not exist. */
        md = FILE_MODE_READ | FILE_MODE_WRITE;
    } else if ( 0 == strcmp(mode, "a") || 0 == strcmp(mode, "ab") ) {
        /* For writing.  Create if not exist. */
        md = FILE_MODE_WRITE | FILE_MODE_APPEND;
    } else if ( 0 == strcmp(mode, "a+") || 0 == strcmp(mode, "ab+") ) {
        /* For reading and writing.  Create if not exist. */
        md = FILE_MODE_READ | FILE_MODE_WRITE | FILE_MODE_APPEND;
    } else {
        /* Invalid mode */
        return NULL;
    }

    /* Allocate FILE data structure */
    f = malloc(sizeof(FILE));
    if ( NULL == f ) {
        return NULL;
    }
    f->fd = fildes;

    /* Allocate input buffer */
    f->ibuf.buf = malloc(FILE_BUFFER_SIZE);
    if ( NULL == f->ibuf.buf ) {
        free(f);
        return NULL;
    }
    f->ibuf.pos = 0;
    f->ibuf.sz = FILE_BUFFER_SIZE;

    /* Allocate output buffer */
    f->obuf.buf = malloc(FILE_BUFFER_SIZE);
    if ( NULL == f->obuf.buf ) {
        free(f->ibuf.buf);
        free(f);
        return NULL;
    }
    f->obuf.pos = 0;
    f->obuf.sz = FILE_BUFFER_SIZE;

    /* Mode */
    f->mode = md;

    /* End-of-File flag */
    f->eof = 0;

    /* Error */
    f->error = 0;

    return f;
}

/*
 * fclose
 */
int
fclose(FILE *stream)
{
    int ret;
    ret = close(stream->fd);
    if ( 0 != ret ) {
        return ret;
    }
    free(stream->ibuf.buf);
    free(stream->obuf.buf);
    free(stream);

    return 0;
}

/*
 * ferror
 */
int
ferror(FILE *stream)
{
    return stream->error;
}

/*
 * clearerr
 */
void
clearerr(FILE *stream)
{
    stream->error = 0;
}

/*
 * fgets
 */
char *
fgets(char * __restrict__ str, int size, FILE * __restrict__ stream)
{
    ssize_t sz;
    ssize_t i;
    size_t pos;

    pos = 0;

    /* Loop until encounter newline */
    for ( ;; ) {
        /* Check EOF */
        if ( stream->eof ) {
            if ( pos ) {
                return str;
            } else {
                return NULL;
            }
        }

        /* Read from the buffer, first */
        for ( i = 0; i < (ssize_t)stream->ibuf.pos; i++ ) {
            str[pos] = stream->ibuf.buf[i];
            pos++;
            if ( '\n' == stream->ibuf.buf[i] || pos == (size_t)size - 1 ) {
                str[pos] = '\0';
                stream->ibuf.pos = stream->ibuf.pos - i - 1;
                if ( 0 != stream->ibuf.pos ) {
                    memmove(stream->ibuf.buf, stream->ibuf.buf + i + 1,
                            stream->ibuf.pos);
                }

                return str;
            }
        }

        /* All buffer is consumed. */
        stream->ibuf.pos = 0;

        /* Read from the buffer to the file */
        sz = read(stream->fd, stream->ibuf.buf, stream->ibuf.sz);
        if ( 0 == sz ) {
            /* EOF */
            stream->eof = 1;

            if ( pos ) {
                return str;
            } else {
                return NULL;
            }
        } else if ( sz < 0 ) {
            /* Error: FIXME */
            stream->error = -1;

            if ( pos ) {
                return str;
            } else {
                return NULL;
            }
        } else {
            stream->ibuf.pos = sz;
        }
    }
}

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
 * fwrite
 */
size_t
fwrite(const void *__restrict__ ptr, size_t size, size_t ntimes,
       FILE *__restrict__ stream)
{
    ssize_t i;
    size_t nw;
    size_t sz;

    nw = 0;
    for ( i = 0; i < (ssize_t)ntimes; i++ ) {
        sz = write(stream->fd, ptr, size);
        ptr += sz;
        nw += sz;
        if ( sz != nw ) {
            return nw;
        }
    }

    return nw;
}

/*
 * fputs
 */
int
fputs(const char *__restrict__ s, FILE *__restrict__ stream)
{
    ssize_t sz;
    size_t len;

    len = strlen(s);

    sz = write(stream->fd, s, len);
    if ( sz <= 0 ) {
        return EOF;
    } else {
        return 0;
    }
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

int
puts(const char *s)
{
    int ret;

    ret = fputs(s, stdout);
    if ( ret < 0 ) {
        return ret;
    }
    putchar('\n');

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
