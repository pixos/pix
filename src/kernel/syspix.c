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
#include <unistd.h>
#include <sys/syscall.h>
#include "kernel.h"

/*
 * Allocate packet map
 */
void *
sys_pix_pmap(size_t len)
{
    void *paddr;
    int order;

    /* Compute the order of the buddy system to be allocated */
    order = bitwidth(DIV_CEIL(len, SUPERPAGESIZE));

    /* Allocate physical memory */
    paddr = pmem_prim_alloc_pages(PMEM_ZONE_LOWMEM, order);
    if ( NULL == paddr ) {
        /* Could not allocate physical memory */
        return NULL;
    }

    /* FIXME: Map virtual memory to this physical memory */

    return NULL;
}

/*
 * Launch processor-mapped tasks
 */
void arch_pix_task(int id, struct ktask *t);
int
sys_pix_create_jobs(void *(*start_routine)(void *))
{
    struct ktask *t;
    struct ktask *nt;

    /* Get the current process */
    t = this_ktask();
    if ( NULL == t || NULL == t->proc ) {
        return -1;
    }

    /* Create a task */
    nt = task_create(t->proc, start_routine);
    if ( NULL == nt ) {
        /* Could not create a new task */
        return -1;
    }

    arch_pix_task(1, nt);

#if 0
    struct ktask_list *l;

    /* Kernel task list entry */
    l = kmalloc(sizeof(struct ktask_list));
    if ( NULL == l ) {
        return - 1;
    }

    /* Kernel task (running) */
    l->ktask = nt;
    l->next = NULL;
    /* Push */
    if ( NULL == g_ktask_root->r.head ) {
        g_ktask_root->r.head = l;
        g_ktask_root->r.tail = l;
    } else {
        g_ktask_root->r.tail->next = l;
        g_ktask_root->r.tail = l;
    }
#endif

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
