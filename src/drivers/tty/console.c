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
#include <limits.h>
#include <fcntl.h>
#include <machine/sysarch.h>
#include <mki/driver.h>
#include "tty.h"

#define VIDEO_RAM       0x000b8000ULL
#define VIDEO_PORT      0x3d4
#define VIDEO_BUFSIZE   4096

/*
 * Initialize the data structure for console driver
 */
int
console_init(struct console *con, const char *ttyname)
{
    int ret;
    ssize_t i;
    struct driver_mapped_device *dev;

    /* Initialize keyboard */
    ret = kbd_init(&con->kbd);
    if ( ret < 0 ) {
        return -1;
    }

    /* Memory mapped I/O */
    con->video.vram = driver_mmap((void *)VIDEO_RAM, 4096);
    if ( NULL == con->video.vram ) {
        return -1;
    }
    /* Reset the video */
    for ( i = 0; i < 80 * 25; i++ ) {
        *(con->video.vram + i) = 0x0f00;
    }

    /* Register the interrupt handler */
    driver_register_irq_handler(1, NULL);

    /* Register console device as a character device */
    dev = driver_register_device(ttyname, 0);
    con->dev = dev;

    con->screen.width = 80;
    con->screen.height = 25;
    con->screen.pos = 0;

    /* Set line buffer */
    con->line.pos = 0;

    return 0;
}

/*
 * Update the position of the cursor
 */
static void
_update_cursor(int pos)
{
    struct sysarch_io io;

    /* Low */
    io.port = VIDEO_PORT;
    io.data = ((pos & 0xff) << 8) | 0x0f;
    sysarch(SYSARCH_OUTW, &io);
    /* High */
    io.port = VIDEO_PORT;
    io.data = (((pos >> 8) & 0xff) << 8) | 0x0e;
    sysarch(SYSARCH_OUTW, &io);
}

static void
_console_putc(struct console *con, int c)
{
    off_t line;
    off_t col;
    size_t n;

    if ( '\n' == c ) {
        line = con->screen.pos / con->screen.width;
        col = con->screen.pos % con->screen.width;
        /* # of characters to put */
        n = con->screen.width - col;
        if ( line + 1 == con->screen.height ) {
            /* Need to scroll the buffer */
            memmove(con->video.vram,
                    con->video.vram + con->screen.width,
                    con->screen.width * (con->screen.height - 1) * 2);
            /* Clear last line */
            memset(con->video.vram + con->screen.width
                   * (con->screen.height - 1), 0x00, con->screen.width * 2);
            con->screen.pos -= col;
        } else {
            con->screen.pos += n;
        }
        _update_cursor(con->screen.pos);
    } else {
        con->video.vram[con->screen.pos] = 0x0f00 | (uint16_t)c;
        con->screen.pos++;
        _update_cursor(con->screen.pos);
    }
}

/*
 * Process console I/O
 */
int
console_proc(struct console *con)
{
    int c;

    /* Write to the device */
    while ( (c = driver_chr_obuf_getc(con->dev)) >= 0 ) {
        _console_putc(con, c);
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
