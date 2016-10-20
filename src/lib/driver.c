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

#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <mki/driver.h>

/* in libcasm.s */
unsigned long long syscall(int, ...);

/*
 * Request IRQ and register an interrupt handler
 */
int
driver_register_irq_handler(int irq, void *func)
{
    struct sysdriver_handler handler;

    handler.nr = irq;
    handler.handler = func;

    return syscall(SYS_driver, SYSDRIVER_REG_IRQ, &handler);
}

/*
 * Register a device to devfs
 */
struct driver_mapped_device *
driver_register_device(const char *name, int flags)
{
    struct sysdriver_devfs req;
    int ret;

    req.name = name;
    req.flags = flags;

    ret = syscall(SYS_driver, SYSDRIVER_REG_DEV, &req);
    if ( ret < 0 ) {
        return NULL;
    }

    return req.dev;
}

/*
 * Allocate virtual pages that are mapped to the physical memory space
 * specified by addr and length
 */
void *
driver_mmap(void *addr, size_t length)
{
    struct sysdriver_mmap_req req;
    int ret;

    req.addr = addr;
    req.length = length;
    ret = syscall(SYS_driver, SYSDRIVER_MMAP, &req);
    if ( ret < 0 ) {
        return NULL;
    }

    return req.vaddr;
}

/*
 * Invoke an interrupt
 */
void
driver_interrupt(struct driver_mapped_device *dev)
{
    syscall(SYS_driver, SYSDRIVER_INTERRUPT, dev);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
