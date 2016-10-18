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

#include <aos/const.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <machine/sysarch.h>
#include <mki/driver.h>
#include "kernel.h"

static int
_sysdriver_reg_irq(struct ktask *t, struct proc *proc, void *args)
{
    struct sysdriver_handler *s;

    s = (struct sysdriver_handler *)args;
    g_intr_table->ivt[IV_IRQ(s->nr)].f = s->handler;
    g_intr_table->ivt[IV_IRQ(s->nr)].proc = proc;

    return 0;
}

static int
_sysdriver_mmap(struct ktask *t, struct proc *proc, void *args)
{
    struct sysdriver_mmap_req *req;
    int order;
    void *paddr;
    void *vaddr;
    ssize_t i;
    int ret;

    req = (struct sysdriver_mmap_req *)args;

    /* Allocate virtual memory region */
    order = bitwidth(DIV_CEIL(req->length, PAGESIZE));
    vaddr = vmem_buddy_alloc_pages(proc->vmem, order);
    if ( NULL == vaddr ) {
        return -1;
    }

    paddr = req->addr;
    for ( i = 0; i < (ssize_t)DIV_CEIL(req->length, PAGESIZE); i++ ) {
        ret = arch_vmem_map(proc->vmem,
                            (void *)(vaddr + PAGESIZE * i),
                            paddr + PAGESIZE * i, VMEM_USABLE | VMEM_USED);
        if ( ret < 0 ) {
            vmem_free_pages(proc->vmem, vaddr);
            return -1;
        }
    }
    req->vaddr = vaddr;

    return 0;
}

static int
_sysdriver_reg_dev(struct ktask *t, struct proc *proc, void *args)
{
    struct sysdriver_devfs *msg;
    void *paddr;
    void *vaddr;
    int ret;
    ssize_t i;
    struct devfs_entry *ent;
    struct driver_mapped_device *mapped;

    /* Message */
    msg = (struct sysdriver_devfs *)args;

    /* Allocate an entry of devfs */
    ent = kmalloc(sizeof(struct devfs_entry));
    if ( NULL == ent ) {
        return -1;
    }
    kmemset(ent, 0, (sizeof(struct devfs_entry)));
    /* Copy the name of the entry */
    ent->name = kstrdup(msg->name);
    if ( NULL == ent->name ) {
        kfree(ent);
        return -1;
    }
    ent->flags = msg->flags;
    ent->proc = proc;
    /* Prepend this entry to devfs */
    ent->next = g_devfs.head;
    g_devfs.head = ent;

    ent->mapped = kmalloc(PAGESIZE);
    kmemset(ent->mapped, 0, PAGESIZE);

    /* Allocate virtual memory region */
    vaddr = vmem_buddy_alloc_pages(proc->vmem, 0);
    if ( NULL == vaddr ) {
        kfree(ent->name);
        kfree(ent);
        return -1;
    }
    paddr = arch_kmem_addr_v2p(g_kmem, ent->mapped);
    for ( i = 0; i < (ssize_t)1; i++ ) {
        ret = arch_vmem_map(proc->vmem,
                            (void *)(vaddr + PAGESIZE * i),
                            paddr + PAGESIZE * i, VMEM_USABLE | VMEM_USED);
        if ( ret < 0 ) {
            vmem_free_pages(proc->vmem, vaddr);
            return -1;
        }
    }
    msg->dev = vaddr;

    ent->mapped_integrity = vaddr;
    mapped = vaddr;
    mapped->file = ent;

    return 0;
}

static int
_sysdriver_interrupt(struct ktask *t, struct proc *proc, void *args)
{
    struct driver_mapped_device *dev;
    struct devfs_entry *e;
    struct fildes *fd;

    dev = (struct driver_mapped_device *)args;
    e = (struct devfs_entry *)dev->file;
    /* Check the integrity */
    if ( e->mapped_integrity != dev ) {
        char buf[512];
        ksnprintf(buf, 512, "%x %x %x", e, e->mapped, dev);
        panic(buf);
        return -1;
    }

    return 0;
}

/*
 * Driver-related system call
 */
int
sys_driver(int number, void *args)
{
    struct ktask *t;
    struct proc *proc;

    /* Get the current task information */
    t = this_ktask();
    if ( NULL == t ) {
        return -1;
    }
    proc = t->proc;
    if ( NULL == proc ) {
        return -1;
    }

    switch ( number ) {
    case SYSDRIVER_REG_IRQ:
        /* Register an IRQ handler */
        return _sysdriver_reg_irq(t, proc, args);

    case SYSDRIVER_MMAP:
        return _sysdriver_mmap(t, proc, args);

    case SYSDRIVER_REG_DEV:
        return _sysdriver_reg_dev(t, proc, args);

    case SYSDRIVER_INTERRUPT:
        return _sysdriver_interrupt(t, proc, args);

    default:
        ;
    }

    return -1;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
