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

#ifndef _FE_H
#define _FE_H

#include "e1000.h"
#include "e1000e.h"
#include "igb.h"
#include "ixgbe.h"
#include "i40e.h"
#include "hashtable.h"

#define FE_MAX_PORTS            64

#define FE_PKTSZ                (10240 + 128)
#define FE_PKT_HDROFF           512
#define FE_BUFFER_POOL_SIZE     8192

#define FE_QLEN                 512

#define FE_MEMSIZE_FOR_DESCS    (1ULL << 24)


/*
 * Driver type
 */
enum fe_driver_type {
    FE_DRIVER_INVALID = -1,
    FE_DRIVER_KERNEL,
    FE_DRIVER_E1000,
    FE_DRIVER_IXGBE,
};

/*
 * Packet buffer header
 */
struct fe_pkt_buf_hdr {
    struct fe_pkt_buf_hdr *next;
    int refs;
};

/*
 * Buffer pool (exclusive per CPU)
 */
struct fe_buffer_pool {
    /* Linked list */
    struct fe_pkt_buf_hdr *head;
    /* Offset to calculate the physical address from the virtual address */
    uint64_t v2poff;
} __attribute__ ((aligned(128)));

/*
 * Kernel ring buffer
 */
struct fe_kernel_ring {
    /* Packets */
    void *pkts;
    /* Buffers */
    struct fe_pkt_buf_hdr **bufs;
    /* Head/tail */
    volatile uint16_t head;
    volatile uint16_t tail;
    uint16_t len;
    /* Queue information */
    uint16_t rsvd;
};

/*
 * Driver
 */
struct fe_driver_rx {
    /* Driver type */
    enum fe_driver_type driver;
    union {
        struct fe_kernel_ring kernel;
        struct e1000_rx_ring e1000;
        struct ixgbe_rx_ring ixgbe;
    } u;
};
struct fe_driver_tx {
    /* Driver type */
    enum fe_driver_type driver;
    union {
        struct fe_kernel_ring kernel;
        struct e1000_tx_ring e1000;
        struct ixgbe_tx_ring ixgbe;
    } u;
};

/*
 * Data per task
 */
struct fe_task {
    /* CPU ID (for exclusive processor), or -1 for kernel */
    int cpuid;

    /* Buffer pool */
    struct fe_buffer_pool pool;

    /* Handling Rx queues */
    struct {
        uint64_t bitmap;
        struct fe_driver_rx *rings;
    } rx;

    /* Handling Tx queues */
    struct {
        /* Port #(fe->nports) => kernel */
        struct fe_driver_tx *rings;
    } tx;

    /* Kernel Tx */
    struct fe_kernel_ring *ktx;

    /* Pointer to the next task */
    struct fe_task *next;
};

/*
 * Physical port
 */
struct fe_device {
    /* NUMA domain */
    int domain;
    /* Driver */
    enum fe_driver_type driver;
    /* Device data */
    union {
        struct e1000_device *e1000;
        struct ixgbe_device *ixgbe;
    } u;
    /* Type; exclusive or kernel */
    int fastpath;
};

/*
 * Forwarding engine
 */
struct fe {
    /* Forwarding/Action database */
    void *fdb;

    /* Processors */
    int ncpus;

    /* Exclusive processors */
    int nxcpu;

    /* Ports */
    size_t nports;
    struct fe_device *ports[FE_MAX_PORTS];

    /* Tickful CPU task */
    struct fe_task *tftask;
    /* Exclusive CPU tasks (linked list) */
    struct fe_task *extasks;

    /* Memory space for descriptors */
    struct {
        void *vaddr;
        size_t len;
        uint64_t v2poff;
        void *free;
    } mem;
};

/*
 * Get a packet to the buffer pool
 */
static __inline__ struct fe_pkt_buf_hdr *
fe_get_buffer(struct fe_task *fet)
{
    struct fe_pkt_buf_hdr *pkt;

    pkt = fet->pool.head;
    if ( NULL != fet->pool.head ) {
        fet->pool.head = fet->pool.head->next;
    }

    return pkt;
}

/*
 * Resolve physical address of the packet buffer
 */
static __inline__ void *
fe_v2p(struct fe_task *fet, void *pkt)
{
    return pkt + fet->pool.v2poff;
}

/*
 * Release a packet to the buffer pool
 */
static __inline__ void
fe_release_buffer(struct fe_task *fet, struct fe_pkt_buf_hdr *pkt)
{
    pkt->next = fet->pool.head;
    fet->pool.head = pkt;
}

/*
 * Collect buffer from Tx
 */
static __inline__ int
fe_collect_buffer(struct fe_task *t, struct fe_driver_tx *tx)
{
    int ret;
    struct fe_pkt_buf_hdr *hdr;

    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        break;
    case FE_DRIVER_E1000:
        ret = e1000_collect_buffer(&tx->u.e1000, (void **)&hdr);
        if ( ret > 0 ) {
            fe_release_buffer(t, hdr);
        }
        return ret;
        break;
    case FE_DRIVER_IXGBE:
        break;
    default:
        ;
    }

    return -1;
}



/*
 * Abstracted API for each driver
 */

/*
 * The number of supported Tx queues
 */
static __inline__ int
fe_driver_max_tx_queues(struct fe_device *dev)
{
    switch ( dev->driver ) {
    case FE_DRIVER_E1000:
        return e1000_max_tx_queues(dev->u.e1000);
    case FE_DRIVER_IXGBE:
        return ixgbe_max_tx_queues(dev->u.ixgbe);
    default:
        ;
    }

    return 0;
}

static __inline__ int
fe_driver_setup_rx_ring(struct fe_device *dev, struct fe_driver_rx *rx, void *m,
                        uint64_t v2poff, uint16_t qlen)
{
    int ret;

    ret = -1;
    rx->driver = dev->driver;
    switch ( dev->driver ) {
    case FE_DRIVER_KERNEL:
        ret = -1;
        break;
    case FE_DRIVER_E1000:
        ret = e1000_setup_rx_ring(dev->u.e1000, &rx->u.e1000, m, v2poff, qlen);
        break;
    case FE_DRIVER_IXGBE:
        ret = ixgbe_setup_rx_ring(dev->u.ixgbe, &rx->u.ixgbe, m, v2poff, qlen);
        break;
    default:
        ret = -1;
    }

    return ret;
}
static __inline__ int
fe_driver_setup_tx_ring(struct fe_device *dev, struct fe_driver_tx *tx, void *m,
                        uint64_t v2poff, uint16_t qlen)
{
    int ret;

    ret = -1;
    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        ret = -1;
        break;
    case FE_DRIVER_E1000:
        ret = e1000_setup_tx_ring(dev->u.e1000, &tx->u.e1000, m, v2poff, qlen);
        break;
    case FE_DRIVER_IXGBE:
        ret = ixgbe_setup_tx_ring(dev->u.ixgbe, &tx->u.ixgbe, m, v2poff, qlen);
        break;
    default:
        ret = -1;
    }

    return ret;
}

static __inline__ int
fe_driver_calc_rx_ring_memsize(struct fe_driver_rx *rx, uint16_t qlen)
{
    int ret;

    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        ret = (sizeof(struct fe_pkt_buf_hdr *) + sizeof(void *)) * qlen;
        break;
    case FE_DRIVER_E1000:
        ret = e1000_calc_rx_ring_memsize(&rx->u.e1000, qlen);
        break;
    case FE_DRIVER_IXGBE:
        ret = ixgbe_calc_rx_ring_memsize(&rx->u.ixgbe, qlen);
        break;
    default:
        ret = -1;
    }

    return ret;
}

static __inline__ int
fe_driver_calc_tx_ring_memsize(struct fe_driver_tx *tx, uint16_t qlen)
{
    int ret;

    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        ret = (sizeof(struct fe_pkt_buf_hdr *) + sizeof(void *)) * qlen;
        break;
    case FE_DRIVER_E1000:
        ret = e1000_calc_tx_ring_memsize(&tx->u.e1000, qlen);
        break;
    case FE_DRIVER_IXGBE:
        ret = ixgbe_calc_tx_ring_memsize(&tx->u.ixgbe, qlen);
        break;
    default:
        ret = -1;
    }

    return ret;
}

static __inline__ int
fe_driver_rx_refill(struct fe_task *t, struct fe_driver_rx *rx)
{
    struct fe_pkt_buf_hdr *pkt;
    void *pa;

    pkt = fe_get_buffer(t);
    if ( NULL == pkt ) {
        return -1;
    }
    pa = fe_v2p(t, pkt);

    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        fe_release_buffer(t, pkt);
        break;
    case FE_DRIVER_E1000:
        return e1000_rx_refill(&rx->u.e1000, pa + FE_PKT_HDROFF, pkt);
    case FE_DRIVER_IXGBE:
        fe_release_buffer(t, pkt);
        break;
    default:
        fe_release_buffer(t, pkt);
        ;
    }

    return 0;
}

static __inline__ int
fe_driver_rx_fill_all(struct fe_task *t, struct fe_driver_rx *rx)
{
    int n;
    int r;

    n = 0;
    while ( (r = fe_driver_rx_refill(t, rx)) > 0 ) {
        n += r;
    }

    return n;
}


static __inline__ void
fe_driver_rx_commit(struct fe_driver_rx *rx)
{
    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        break;
    case FE_DRIVER_E1000:
        e1000_rx_commit(&rx->u.e1000);
        break;
    case FE_DRIVER_IXGBE:
        break;
    default:
        ;
    }
}

static __inline__ int
fe_driver_rx_dequeue(struct fe_driver_rx *rx, struct fe_pkt_buf_hdr **hdr,
                     void **pkt)
{
    int ret;

    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        break;
    case FE_DRIVER_E1000:
        ret = e1000_rx_dequeue(&rx->u.e1000, (void **)hdr);
        if ( ret > 0 ) {
            *pkt = (void *)*hdr + FE_PKT_HDROFF;
        }
        return ret;
    case FE_DRIVER_IXGBE:
        break;
    default:
        ;
    }

    return -1;
}

static __inline__ int
fe_driver_tx_enqueue(struct fe_driver_tx *tx, void *pkt, void *hdr,
                     size_t length)
{
    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        break;
    case FE_DRIVER_E1000:
        return e1000_tx_enqueue(&tx->u.e1000, pkt, hdr, length);
    case FE_DRIVER_IXGBE:
        break;
    default:
        ;
    }

    return -1;
}

static __inline__ void
fe_driver_tx_commit(struct fe_driver_tx *tx)
{
    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        break;
    case FE_DRIVER_E1000:
        e1000_tx_commit(&tx->u.e1000);
        break;
    case FE_DRIVER_IXGBE:
        break;
    default:
        ;
    }
}

#endif /* _FE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
