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

#include <aos/const.h>
#include "kernel.h"

void
sys_task_switch(void);

/*
 * read
 */
ssize_t
devfs_read(struct fildes *fildes, void *buf, size_t nbyte)
{
    struct ktask *t;
    struct devfs_entry *ent;
    struct driver_device_fifo *q;
    ssize_t len;
    struct ktask_list_entry *tle;

    /* Get the current process */
    t = this_ktask();
    if ( NULL == t ) {
        return -1;
    }

    /* Obtain the file-system-specific data structure */
    ent = (struct devfs_entry *)fildes->data;

    switch ( ent->type ) {
    case DEVFS_CHAR:
        /* Character device */
        q = &ent->mapped->dev.chr.ibuf;
        if ( q->head == q->tail ) {
            /* Add this task to the blocking task list for this file descriptor
             */
            tle = kmalloc(sizeof(struct ktask_list_entry));
            if ( NULL == tle ) {
                return -1;
            }
            tle->ktask = t;
            tle->next = fildes->blocking_tasks;
            fildes->blocking_tasks = tle;

            /* Buffer is empty, then block this process */
            t->state = KTASK_STATE_BLOCKED;

            /* Switch this task to another */
            sys_task_switch();
        }
        len = 0;
        while ( q->head != q->tail && len < (ssize_t)nbyte ) {
            *(char *)(buf + len) = q->buf[q->head];
            len++;
            q->head++;
            q->head = q->head < 512 ? q->head : 0;
        }
        return len;

    case DEVFS_BLOCK:
        /* Block device */
        return -1;
    }

    return -1;
}

/*
 * Write
 */
ssize_t
devfs_write(struct fildes *fildes, const void *buf, size_t nbyte)
{
    struct ktask *t;
    struct devfs_entry *ent;
    struct driver_device_fifo *q;
    ssize_t len;
    off_t next_tail;
    struct ktask *tmp;

    /* Get the current process */
    t = this_ktask();
    if ( NULL == t ) {
        return -1;
    }

    /* Obtain the file-system-specific data structure */
    ent = (struct devfs_entry *)fildes->data;

    switch ( ent->type ) {
    case DEVFS_CHAR:
        /* Character device */
        q = &ent->mapped->dev.chr.obuf;

        next_tail = q->tail + 1;
        next_tail = next_tail < 512 ? next_tail : 0;
        if ( q->head == next_tail ) {
            /* Buffer is full. FIXME: Implement blocking write */
            return 0;
        }
        len = 0;
        while ( q->head != next_tail && len < (ssize_t)nbyte ) {
            q->buf[q->tail] = *(char *)(buf + len);
            len++;
            q->tail = next_tail;
            next_tail = next_tail + 1 < 512 ? next_tail + 1 : 0;
        }
        /* Wake up the driver */
        tmp = ent->proc->tasks;
        while ( NULL != tmp ) {
            tmp->state = KTASK_STATE_READY;
            tmp = tmp->next;
        }
        return len;

    case DEVFS_BLOCK:
        /* Block device */
        return -1;
    }

    return -1;
}

/*
 * Seek
 */
off_t
devfs_lseek(struct fildes *fildes, off_t offset, int whence)
{
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
