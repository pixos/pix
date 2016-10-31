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
#include <fcntl.h>
#include <sys/pix.h>

#ifndef PIX_VERSION
#define PIX_VERSION "unknown"
#endif

unsigned long long syscall(int, ...);

static void
show_cpu(void)
{
    struct syspix_cpu_table cputable;
    int n;
    ssize_t i;
    char buf[512];

    n = syscall(SYS_pix_cpu_table, SYSPIX_LDCTBL, &cputable);
    if ( n < 0 ) {
        fputs("Could not get processor list.\n", stderr);
        return;
    }

    for ( i = 0; i < PIX_MAX_CPU; i++ ) {
        if ( cputable.cpus[i].present ) {
            const char *mode;
            switch ( cputable.cpus[i].type ) {
            case SYSPIX_CPU_TICKFUL:
                mode = "tickful";
                break;
            case SYSPIX_CPU_EXCLUSIVE:
                mode = "exclusive";
                break;
            default:
                mode = "unknown";
            }
            snprintf(buf, sizeof(buf),
                     "Processor #%ld is present at Node %d in %s mode.\n", i,
                     cputable.cpus[i].domain, mode);
            fputs(buf, stdout);
        }
    }
}

/*
 * Entry point for pash
 */
int
main(int argc, char *argv[])
{
    char buf[256];

    /* Print out welcome message */
    printf("Welcome to Packet Information Chaining Service (pix)\n");
    printf("pix version: %s\n", PIX_VERSION);

    putchar('>');
    putchar(' ');

    while ( 1 ) {
        if ( fgets(buf, sizeof(buf), stdin) ) {
            if ( 0 == strcmp("show cpu\n", buf) ) {
                show_cpu();
            }
            putchar('>');
            putchar(' ');
        }
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
