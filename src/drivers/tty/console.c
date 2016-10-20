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
#include <time.h>
#include <mki/driver.h>
#include "tty.h"

#define VIDEO_RAM       0x000b8000ULL

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
