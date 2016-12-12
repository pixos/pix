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

#ifndef _SYS_PIX_H
#define _SYS_PIX_H

#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>

/* Must be consistent with MAX_PROCSSORS in kernel.h */
#define PIX_MAX_CPU             256

#define PIX_PKT_SIZE            (10240 + 128)
#define PIX_PKT_HDROFF          512

#define SYSPIX_LDCTBL           1
#define SYSPIX_STCTBL           2

#define SYSPIX_CPU_TICKFUL      1
#define SYSPIX_CPU_EXCLUSIVE    2

/*
 * Packet buffer header
 */
struct pix_pkt_buffer_hdr {
    /* Pointer to the next packet buffer */
    struct pix_pkt_buffer_hdr *next;
    /* Physical address */
    void *paddr;
    /* Reference counter */
    int refs;
    /* Inheritted from fpp */
    int port;
} __attribute__ ((packed));

/*
 * Buffer pool
 */
struct pix_buffer_pool {
    /* Linked list */
    struct pix_pkt_buffer_hdr *head;
    /* Offset to calculate the physical address from the virtual address */
    uint64_t v2poff;
};

/*
 * CPU configuration
 */
struct syspix_cpu_config {
    uint8_t present;
    uint8_t type;
    int domain;
};

/*
 * CPU configuration table
 */
struct syspix_cpu_table {
    struct syspix_cpu_config cpus[PIX_MAX_CPU];
};

/* Prototype declarations */
int pix_ldcpuconf(struct syspix_cpu_table *);
struct pix_buffer_pool * pix_create_buffer_pool(size_t);
void *pix_malloc(size_t);

#endif /* _SYS_PIX_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
