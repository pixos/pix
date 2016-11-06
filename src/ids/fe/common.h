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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

/*
 * Busy-wait for d_usec microseconds
 */
static __inline__ void
busywait(uint64_t d_usec)
{
    struct timeval tv;
    uint64_t usec0;
    uint64_t usec;

    /* Get the current time stamp */
    gettimeofday(&tv, NULL);
    usec0 = tv.tv_sec * 1000000 + tv.tv_usec;

    /* Loop for d_usec+ microseconds */
    for ( ;; ) {
        gettimeofday(&tv, NULL);
        usec = tv.tv_sec * 1000000 + tv.tv_usec;
        if ( usec >= usec0 + d_usec ) {
            break;
        }
    }
}

/*
 * Read data from a 32-bit register via MMIO
 */
static __inline__ uint32_t
rd32(void *mmio, uint64_t reg)
{
    return *(volatile uint32_t *)(mmio + reg);
}

/*
 * Write data to a 32-bit register via MMIO
 */
static __inline__ void
wr32(void *mmio, uint64_t reg, volatile uint32_t val)
{
    *(volatile uint32_t *)(mmio + reg) = val;
}

#endif /* _COMMON_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
