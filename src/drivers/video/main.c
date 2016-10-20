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
#include <sys/syscall.h>
#include <time.h>
#include <mki/driver.h>

#define VIDEO_RAM       0x000b8000ULL

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    struct timespec tm;
    uint16_t *vram;
    char buf[512];
    ssize_t i;
    struct driver_mapped_device *dev;

    /* Memory mapped I/O */
    vram = driver_mmap((void *)VIDEO_RAM, 4096);
    if ( NULL == vram ) {
        exit(EXIT_FAILURE);
    }
    /* Reset the video */
    for ( i = 0; i < 80 * 25; i++ ) {
        *(vram + i) = 0x0f00;
    }

    /* Register this driver to devfs */
    dev = driver_register_device("video", 0);
    if ( NULL == dev ) {
        exit(EXIT_FAILURE);
    }

    tm.tv_sec = 1;
    tm.tv_nsec = 0;
    while ( 1 ) {
#if 0
        snprintf(buf, 512, "Sleeping vram driver: %p.", vram);
        for ( i = 0; i < 80 * 25; i++ ) {
            *(vram + i) = 0x0f00;
        }
        for ( i = 0; i < (ssize_t)strlen(buf); i++ ) {
            *(vram + i) = 0x0f00 | (uint16_t)((char *)buf)[i];
        }
#endif
        nanosleep(&tm, NULL);
    }

    exit(0);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
