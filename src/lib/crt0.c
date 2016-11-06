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
#include <unistd.h>

#if !defined(TEST) || !TEST
int main(int argc, char *argv[]);

int libc_init(void);

/*
 * Entry point to a process
 */
void
entry(int argc, char *argv[])
{
    int ret;

    /* Initialize libc */
    ret = libc_init();
    if ( ret < 0 ) {
        exit(ret);
        while ( 1 ) {}
    }

    /* Prepare stdio/stdout/stderr */
    stdin = malloc(sizeof(FILE));
    if ( NULL == stdin ) {
        exit(EXIT_FAILURE);
    }
    stdin->fd = STDIN_FILENO;
    stdout = malloc(sizeof(FILE));
    if ( NULL == stdout ) {
        free(stdin);
        exit(EXIT_FAILURE);
    }
    stdout->fd = STDOUT_FILENO;
    stderr = malloc(sizeof(FILE));
    if ( NULL == stderr ) {
        free(stdin);
        free(stdout);
        exit(EXIT_FAILURE);
    }
    stderr->fd = STDERR_FILENO;

    ret = main(argc, argv);
    exit(ret);

    while ( 1 ) {}
}
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
