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

#ifndef _TTY_H
#define _TTY_H

#include <stdint.h>
#include <sys/types.h>
#include <termios.h>
#include "kbd.h"

#define TTY_LINEBUFSIZE 4096

/*
 * Line buffer
 */
struct tty_line_buffer {
    /* Cursor */
    off_t cur;
    /* Length */
    size_t len;
    /* Buffer */
    char buf[TTY_LINEBUFSIZE];
};

/*
 * TTY
 */
struct tty {
    struct tty_line_buffer lbuf;
    struct termios term;
};

/*
 * Video
 */
struct video {
    uint16_t *vram;
};

/*
 * Console
 */
struct console {
    /* Keyboard */
    struct kbd kbd;
    /* Video */
    struct video video;
    /* Console (screen) buffer */
    struct {
        /* Buffer size */
        size_t size;
        /* Width */
        size_t width;
        /* Height */
        size_t height;
        /* Cursor position */
        off_t cur;
        /* End-of-buffer */
        off_t eob;
        /* Line buffer marker */
        off_t lmark;
    } screen;
    /* Character device */
    struct driver_mapped_device *dev;
};

/*
 * Serial
 */
struct serial {
    int port;
    int irq;
    struct driver_mapped_device *dev;
};

int tty_line_buffer_init(struct tty_line_buffer *);
int tty_line_buffer_putc(struct tty_line_buffer *, int c);
int console_init(struct console *, const char *);
int console_proc(struct console *, struct tty *);
int serial_init(struct serial *, int, const char *);
int serial_proc(struct serial *, struct tty *);

#endif /* _TTY_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
