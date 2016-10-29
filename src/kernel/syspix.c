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
int
sys_pix_bufpool(size_t len, void **pa, void **va)
{
    struct ktask *t;
    struct proc *proc;
    void *paddr;
    void *vaddr;
    ssize_t i;
    int order;
    int ret;

    /* Get the current task information */
    t = this_ktask();
    if ( NULL == t ) {
        return -1;
    }
    proc = t->proc;
    if ( NULL == proc ) {
        return -1;
    }

    /* Compute the order of the buddy system to be allocated */
    order = bitwidth(DIV_CEIL(len, SUPERPAGESIZE));

    /* Allocate virtual memory */
    vaddr = vmem_buddy_alloc_superpages(proc->vmem, order);
    if ( NULL == vaddr ) {
        return -1;
    }

    /* Allocate physical memory */
    paddr = pmem_prim_alloc_pages(PMEM_ZONE_LOWMEM, order);
    if ( NULL == paddr ) {
        /* Could not allocate physical memory */
        vmem_free_pages(proc->vmem, vaddr);
        return -1;
    }

    for ( i = 0; i < (ssize_t)DIV_CEIL(len, SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(proc->vmem,
                            (void *)(vaddr + SUPERPAGESIZE * i),
                            paddr + SUPERPAGESIZE * i,
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            pmem_prim_free_pages(paddr);
            vmem_free_pages(proc->vmem, vaddr);
            return -1;
        }
    }

    *va = vaddr;
    *pa = paddr;

    return 0;
}


/*
 * Get/Set CPU configuration table
 */
int
sys_pix_cpu_table(int req, struct syspix_cpu_table *cputable)
{
    switch ( req ) {
    case SYSPIX_LDCTBL:
        /* Load CPU table */
        return arch_load_cpu_table(cputable);
    case SYSPIX_STCTBL:
        /* Store/Set CPU table */
        return arch_store_cpu_table(cputable);
    }

    return -1;
}

/*
 * Launch processor-mapped tasks
 */
void arch_pix_task(int id, struct ktask *t);
int
sys_pix_create_job(int cpuid, void *(*start_routine)(void *), void *args)
{
    struct ktask *t;
    struct ktask *nt;

    /* Get the current process */
    t = this_ktask();
    if ( NULL == t || NULL == t->proc ) {
        return -1;
    }

    /* Create a task */
    nt = task_create(t->proc, start_routine, args);
    if ( NULL == nt ) {
        /* Could not create a new task */
        return -1;
    }

    /* Launch the task at processor #cpuid */
    arch_pix_task(cpuid, nt);

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
