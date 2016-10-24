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
#include "tty.h"

#define TTY_CONSOLE_PREFIX  "console"
#define TTY_SERIAL_PREFIX  "ttys"

/*
 * Entry point for the tty driver
 */
int
main(int argc, char *argv[])
{
    const char *ttyname;
    char path[PATH_MAX];
    struct timespec tm;
    int ret;
    pid_t pid;
    char *shell_args[] = {"/bin/pash", NULL};
    struct tty tty;

    tm.tv_sec = 0;
    tm.tv_nsec = 100000000;

    /* Check the arguments */
    if ( argc != 2 ) {
        exit(EXIT_FAILURE);
    }
    ttyname = argv[1];
    snprintf(path, PATH_MAX, "/dev/%s", ttyname);

    /* Initialize the tty */
    tty_line_buffer_init(&tty.lbuf);
    tty.term.c_iflag = 0;
    tty.term.c_oflag = 0;
    tty.term.c_cflag = 0;
    tty.term.c_lflag = ECHO;
    tty.term.ispeed = 0;
    tty.term.ospeed = 0;

    /* Check the type */
    if ( 0 == strncmp(ttyname, TTY_CONSOLE_PREFIX,
                      strlen(TTY_CONSOLE_PREFIX)) ) {
        /* Console */
        struct console console;

        console_init(&console, ttyname);
        int fd[3];

        /* Set the serial device as stdin/stdout/stderr */
        fd[0] = open(path, O_RDONLY);
        fd[1] = open(path, O_WRONLY);
        fd[2] = open(path, O_WRONLY);
        (void)fd[0];

        /* fork */
        pid = fork();
        switch ( pid ) {
        case -1:
            /* Error */
            exit(-1);
            break;
        case 0:
            /* The child process */
            ret = execve("/bin/pash", shell_args, NULL);
            if ( ret < 0 ) {
                /* Error */
                return -1;
            }
            break;
        default:
            /* The parent process */
            ;
        }

        for ( ;; ) {
            console_proc(&console, &tty);
            nanosleep(&tm, NULL);
        }
    } else if ( 0 == strncmp(ttyname, TTY_SERIAL_PREFIX,
                             strlen(TTY_SERIAL_PREFIX)) ) {
        /* Serial */
        struct serial serial;
        serial_init(&serial, 0, ttyname);
        int fd[3];

        /* Set the serial device as stdin/stdout/stderr */
        fd[0] = open(path, O_RDONLY);
        fd[1] = open(path, O_WRONLY);
        fd[2] = open(path, O_WRONLY);
        (void)fd[0];

        /* fork */
        pid = fork();
        switch ( pid ) {
        case -1:
            /* Error */
            exit(-1);
            break;
        case 0:
            /* The child process */
            ret = execve("/bin/pash", shell_args, NULL);
            if ( ret < 0 ) {
                /* Error */
                return -1;
            }
            break;
        default:
            /* The parent process */
            ;
        }

        for ( ;; ) {
            serial_proc(&serial, &tty);
            nanosleep(&tm, NULL);
        }
    }

    /* Loop */
    tm.tv_sec = 1;
    tm.tv_nsec = 0;
    for ( ;; ) {
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
