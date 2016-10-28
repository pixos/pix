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

#define SYSDRIVER_DEV_BUFSIZE   8192

struct sysdriver_handler {
    int nr;
    void *handler;
};

struct sysdriver_devfs {
    /* Arguments */
    const char *name;
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
    uint8_t buf[SYSDRIVER_DEV_BUFSIZE];
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
    uint8_t buf[SYSDRIVER_DEV_BUFSIZE];
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
struct driver_mapped_device * driver_register_device(const char *, int);
void * driver_mmap(void *, size_t);
void driver_interrupt(struct driver_mapped_device *);

/*
 * Put one character to the input buffer
 */
static __inline__ int
driver_chr_ibuf_putc(struct driver_mapped_device *dev, int c)
{
    off_t cur;
    off_t next;

    __sync_synchronize();

    cur = dev->dev.chr.ibuf.tail;
    next = cur + 1 < SYSDRIVER_DEV_BUFSIZE ? cur + 1 : 0;

    if  ( dev->dev.chr.ibuf.head == next ) {
        /* Buffer is full */
        return -1;
    }

    dev->dev.chr.ibuf.buf[cur] = c;
    dev->dev.chr.ibuf.tail = next;

    __sync_synchronize();

    return c;
}

/*
 * Get one character from the input buffer
 */
static __inline__ int
driver_chr_ibuf_getc(struct driver_mapped_device *dev)
{
    int c;
    off_t cur;
    off_t next;

    __sync_synchronize();

    if  ( dev->dev.chr.ibuf.head == dev->dev.chr.ibuf.tail ) {
        /* Buffer is empty */
        return -1;
    }
    cur = dev->dev.chr.ibuf.head;
    next = cur + 1 < SYSDRIVER_DEV_BUFSIZE ? cur + 1 : 0;

    c = dev->dev.chr.ibuf.buf[cur];
    dev->dev.chr.ibuf.head = next;

    __sync_synchronize();

    return c;
}

/*
 * Put one character to the output buffer
 */
static __inline__ int
driver_chr_obuf_putc(struct driver_mapped_device *dev, int c)
{
    off_t cur;
    off_t next;

    __sync_synchronize();

    cur = dev->dev.chr.obuf.tail;
    next = cur + 1 < SYSDRIVER_DEV_BUFSIZE ? cur + 1 : 0;

    if  ( dev->dev.chr.obuf.head == next ) {
        /* Buffer is full */
        return -1;
    }

    dev->dev.chr.obuf.buf[cur] = c;
    dev->dev.chr.obuf.tail = next;

    __sync_synchronize();

    return c;
}

/*
 * Get one character from the output buffer
 */
static __inline__ int
driver_chr_obuf_getc(struct driver_mapped_device *dev)
{
    int c;
    off_t cur;
    off_t next;

    __sync_synchronize();

    if  ( dev->dev.chr.obuf.head == dev->dev.chr.obuf.tail ) {
        /* Buffer is empty */
        return -1;
    }
    cur = dev->dev.chr.obuf.head;
    next = cur + 1 < SYSDRIVER_DEV_BUFSIZE ? cur + 1 : 0;

    c = dev->dev.chr.obuf.buf[cur];
    dev->dev.chr.obuf.head = next;

    __sync_synchronize();

    return c;
}

/*
 * Get the queued length for the input buffer of a character device
 */
static __inline__ int
driver_chr_ibuf_length(struct driver_mapped_device *dev)
{
    __sync_synchronize();

    if ( dev->dev.chr.ibuf.tail >= dev->dev.chr.ibuf.head ) {
        return dev->dev.chr.ibuf.tail - dev->dev.chr.ibuf.head;
    } else {
        return SYSDRIVER_DEV_BUFSIZE + dev->dev.chr.ibuf.tail
            - dev->dev.chr.ibuf.head;
    }
}
static __inline__ int
driver_chr_ibuf_available(struct driver_mapped_device *dev)
{
    if ( driver_chr_ibuf_length(dev) >= SYSDRIVER_DEV_BUFSIZE - 1 ) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * Get the queued length for the output buffer of a character device
 */
static __inline__ int
driver_chr_obuf_length(struct driver_mapped_device *dev)
{
    __sync_synchronize();

    if ( dev->dev.chr.obuf.tail >= dev->dev.chr.obuf.head ) {
        return dev->dev.chr.obuf.tail - dev->dev.chr.obuf.head;
    } else {
        return SYSDRIVER_DEV_BUFSIZE + dev->dev.chr.obuf.tail
            - dev->dev.chr.obuf.head;
    }
}
static __inline__ int
driver_chr_obuf_available(struct driver_mapped_device *dev)
{
    if ( driver_chr_obuf_length(dev) >= SYSDRIVER_DEV_BUFSIZE - 1 ) {
        return 0;
    } else {
        return 1;
    }
}

#endif /* _MKI_DRIVER_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
