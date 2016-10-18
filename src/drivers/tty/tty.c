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
#include <limits.h>
#include <fcntl.h>
#include <time.h>

/*
 * Entry point for the tty driver
 */
int
main(int argc, char *argv[])
{
    const char *tty;
    char path[PATH_MAX];
    int fd;
    struct timespec tm;
    int foreground;
    ssize_t rsz;
    char buf[128];

    /* Check the arguments */
    if ( argc != 2 ) {
        exit(EXIT_FAILURE);
    }
    tty = argv[1];
    snprintf(path, PATH_MAX, "/dev/%s", tty);

    tm.tv_sec = 1;
    tm.tv_nsec = 0;
    nanosleep(&tm, NULL);

    /* Open tty file */
    fd = open("/dev/kbd", O_RDWR);
    if ( fd < 0 ) {
        exit(EXIT_FAILURE);
    }

    foreground = 1;
    while ( 1 ) {
        rsz = read(fd, buf, 128);

        snprintf(buf, 128, "test %ld %x", rsz, buf[0]);
        write(STDOUT_FILENO, buf, strlen(buf));

        //nanosleep(&tm, NULL);
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