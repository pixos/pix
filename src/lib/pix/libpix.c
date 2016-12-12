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
#include <sys/pix.h>
#include <sys/syscall.h>

unsigned long long syscall(int, ...);
static void *pix_malloc_ptr = NULL;

/*
 * Load CPU configuration
 */
int
pix_ldcpuconf(struct syspix_cpu_table *cputable)
{
    int n;

    /* Load the CPU table through system call */
    n = syscall(SYS_pix_cpu_table, SYSPIX_LDCTBL, cputable);

    return n;
}

/*
 * Create a buffer pool
 */
struct pix_buffer_pool *
pix_create_buffer_pool(size_t len)
{
    int ret;
    size_t sz;
    uint64_t voff;
    void *paddr;
    void *vaddr;
    void *pkt;
    struct pix_buffer_pool *pool;
    struct pix_pkt_buffer_hdr *hdr;
    struct pix_pkt_buffer_hdr *prev;
    ssize_t i;

    /* Allocate for the buffer pool */
    pool = malloc(sizeof(struct pix_buffer_pool));
    if ( NULL == pool ) {
        return NULL;
    }

    /* Calculate the memory size to be allocated */
    sz = PIX_PKT_SIZE * len;
    ret = syscall(SYS_pix_malloc, sz, &paddr, &vaddr);
    if ( ret < 0 ) {
        free(pool);
        return NULL;
    }
    /* Offset to calculate physical address from the virtual address */
    voff = paddr - vaddr;

    /* Create a buffer pool for the tickful task */
    pkt = vaddr;
    prev = NULL;
    hdr = NULL;
    for ( i = 0; i < (ssize_t)len; i++ ) {
        hdr = (struct pix_pkt_buffer_hdr *)pkt;
        hdr->next = prev;
        hdr->refs = 0;
        prev = hdr;
        pkt += PIX_PKT_SIZE;
    }
    pool->head = hdr;
    pool->v2poff = voff;

    return pool;
}

/*
 * Allocate physically contiguous memory space, e.g., for descriptors.
 */
void *
pix_malloc(size_t sz)
{
    if ( NULL == pix_malloc_ptr ) {
        return NULL;
    }

    return NULL;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
