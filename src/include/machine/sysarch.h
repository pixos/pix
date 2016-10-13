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

#ifndef _MACHINE_SYSARCH_H
#define _MACHINE_SYSARCH_H

#define SYSARCH_INB     1
#define SYSARCH_INW     2
#define SYSARCH_INL     3
#define SYSARCH_OUTB    5
#define SYSARCH_OUTW    6
#define SYSARCH_OUTL    7
#define SYSARCH_CPUMAP  11
#define SYSARCH_RDMSR   32
#define SYSARCH_WRMSR   33
#define SYSARCH_GETCR0  34
#define SYSARCH_SETCR0  35
#define SYSARCH_GETCR4  36
#define SYSARCH_SETCR4  37

struct sysarch_io {
    long long port;
    long long data;
};
struct sysarch_cpumap {
    int id;
};
struct sysarch_msr {
    unsigned long long key;
    unsigned long long value;
};

int sysarch(int, void *);

#endif /* _MACHINE_SYSARCH_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
