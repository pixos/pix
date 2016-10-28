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

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

/*
 * Entry point for the init program
 */
int
main(int argc, char *argv[])
{
    int ret;
    pid_t pid;
    //int stat;
    char *tty_console_args[] = {"/drivers/tty", "console", NULL};
    char *tty_serial0_args[] = {"/drivers/tty", "ttys0", NULL};
    char *pci_args[] = {"/drivers/pci", NULL};
    char *fe_args[] = {"/ids/fe", NULL};
#if 0
    char *e1000_args[] = {"/drivers/e1000", NULL};
#endif

    /* fork */
    pid = fork();
    switch ( pid ) {
    case -1:
        /* Error */
        exit(-1);
        break;
    case 0:
        /* The child process */
        ret = execve("/drivers/tty", tty_console_args, NULL);
        if ( ret < 0 ) {
            /* Error */
            return -1;
        }
        break;
    default:
        /* The parent process */
        ;
    }

    /* fork */
    pid = fork();
    switch ( pid ) {
    case -1:
        /* Error */
        exit(-1);
        break;
    case 0:
        /* The child process */
        ret = execve("/drivers/tty", tty_serial0_args, NULL);
        if ( ret < 0 ) {
            /* Error */
            return -1;
        }
        break;
    default:
        /* The parent process */
        ;
    }

    /* fork */
    pid = fork();
    switch ( pid ) {
    case -1:
        /* Error */
        exit(-1);
        break;
    case 0:
        /* The child process */
        ret = execve("/drivers/pci", pci_args, NULL);
        if ( ret < 0 ) {
            /* Error */
            return -1;
        }
        break;
    default:
        /* The parent process */
        ;
    }

    /* fork */
    pid = fork();
    switch ( pid ) {
    case -1:
        /* Error */
        exit(-1);
        break;
    case 0:
        /* The child process */
        ret = execve("/ids/fe", fe_args, NULL);
        if ( ret < 0 ) {
            /* Error */
            return -1;
        }
        break;
    default:
        /* The parent process */
        ;
    }

    struct timespec tm;
    tm.tv_sec = 1;
    tm.tv_nsec = 0;
    while ( 1 ) {
        nanosleep(&tm, NULL);
    }

#if 0
    /* fork */
    pid = fork();
    switch ( pid ) {
    case -1:
        /* Error */
        exit(-1);
        break;
    case 0:
        /* The child process */
        ret = execve("/drivers/e1000", e1000_args, NULL);
        if ( ret < 0 ) {
            /* Error */
            return -1;
        }
        break;
    default:
        /* The parent process */
        ;
    }
#endif

    //pid = waitpid(pid, &stat, 0);
    while ( 1 ) {
        //write(0, NULL, 0);
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
