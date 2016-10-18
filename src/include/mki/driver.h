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

#ifndef _MKI_DRIVER_H
#define _MKI_DRIVER_H

#include <stdint.h>
#include <unistd.h>

#define SYSDRIVER_REG_IRQ       1
#define SYSDRIVER_UNREG_IRQ     2
#define SYSDRIVER_REG_DEV       3
#define SYSDRIVER_UNREG_DEV     4

#define SYSDRIVER_MMAP          11
#define SYSDRIVER_MUNMAP        12

#define SYSDRIVER_INTERRUPT     20

struct sysdriver_handler {
    int nr;
    void *handler;
};

struct sysdriver_devfs {
    /* Arguments */
    char *name;
    int flags;
    /* Return value(s) */
    struct driver_mapped_device *dev;
};

struct sysdriver_mmap_req {
    void *addr;
    size_t length;
    void *vaddr;
};
/*
 * Ring buffer
 */
struct driver_device_fifo {
    uint8_t buf[512];
    volatile off_t head;
    volatile off_t tail;
};
/*
 * Character device
 */
struct driver_mapped_device_chr {
    /* FIFO */
    struct driver_device_fifo ibuf;
    struct driver_device_fifo obuf;
};
/*
 * Block device
 */
struct driver_mapped_device_blk {
    uint8_t buf[2048];
};
/*
 * Mapped device (also referred from struct devfs_entry)
 */
struct driver_mapped_device {
    union {
        struct driver_mapped_device_chr chr;
        struct driver_mapped_device_blk blk;
    } dev;
    void *file;
};

int driver_register_irq_handler(int, void *);
struct driver_device_chr * driver_register_device(char *, int);
void * driver_mmap(void *, size_t);
void driver_interrupt(struct driver_device_chr *);

#endif /* _MKI_DRIVER_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
