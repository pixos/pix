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
#include "fdb.h"

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
    /* Inheritted from fpp */
    int port;
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
 * Descriptor for kernel ring buffers
 */
struct fe_kernel_desc {
    void *pkt;
    uint16_t length;
    uint16_t port;              /* Outgoing port */
    uint16_t mode;              /* 0: pkt forwarding, 1: control */
    uint16_t rsvd[1];
} __attribute__ ((packed));

/*
 * Kernel ring buffer
 */
struct fe_kernel_ring {
    /* Packets */
    struct fe_kernel_desc *descs;
    /* Buffers */
    struct fe_pkt_buf_hdr **bufs;
    /* Head/tail */
    volatile uint16_t head;     /* Operated from Rx */
    volatile uint16_t tail;     /* Operated from Tx */
    uint16_t tx_head;         /* Managed by Tx for buffer collection */
    uint16_t rx_head;         /* Managed by Rx for buffer release */
    /* Length */
    uint16_t len;
};

/*
 * Driver
 */
struct fe_driver_rx {
    /* Driver type */
    enum fe_driver_type driver;
    /* Port # */
    int port;
    union {
        struct fe_kernel_ring *kernel;
        struct e1000_rx_ring e1000;
        struct ixgbe_rx_ring ixgbe;
    } u;
};
struct fe_driver_tx {
    /* Driver type */
    enum fe_driver_type driver;
    union {
        struct fe_kernel_ring *kernel;
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

    /* Back-link */
    struct fe *fe;

    /* Buffer pool */
    struct fe_buffer_pool pool;

    /* Kernel Tx */
    struct fe_kernel_ring *ktx;

    /* Handling Rx queues */
    struct {
        uint64_t bitmap;
        struct fe_driver_rx *rings;
    } rx;

    /* Handling Tx queues */
    struct {
        /* # of ports */
        struct fe_driver_tx *rings;
    } tx;

    /* Pointer to the next task */
    struct fe_task *next;
};

/*
 * Physical port
 */
struct fe_device {
    /* Port # */
    int port;
    /* NUMA domain */
    int domain;
    /* Last allocated queue # */
    int rxq_last;
    int txq_last;
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
    struct fdb *fdb;

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
 * Abstracted API for each driver
 */

static __inline__ int
fe_kernel_collect_buffer(struct fe_kernel_ring *ring, void **hdr)
{
    if ( ring->tx_head == ring->head ) {
        /* Already collected */
        return 0;
    }

    *hdr = ring->bufs[ring->tx_head];
    __sync_synchronize();
    ring->tx_head = ring->tx_head + 1 < ring->len ? ring->tx_head + 1 : 0;

    return 1;

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
        ret = fe_kernel_collect_buffer(tx->u.kernel, (void **)&hdr);
        if ( ret > 0 ) {
            if ( NULL != hdr ) {
                hdr->refs--;
                if ( hdr->refs <= 0 ) {
                    fe_release_buffer(t, hdr);
                }
            }
        }
        return 0;

    case FE_DRIVER_E1000:
        ret = e1000_collect_buffer(&tx->u.e1000, (void **)&hdr);
        if ( ret > 0 ) {
            hdr->refs--;
            if ( hdr->refs <= 0 ) {
                fe_release_buffer(t, hdr);
            }
        }
        return 0;

    case FE_DRIVER_IXGBE:
        ret = ixgbe_collect_buffer(&tx->u.ixgbe, (void **)&hdr);
        if ( ret > 0 ) {
            hdr->refs--;
            if ( hdr->refs <= 0 ) {
                fe_release_buffer(t, hdr);
            }
        }
        return 0;

    default:
        ;
    }

    return -1;
}

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

/*
 * Setup an Rx ring
 */
static __inline__ int
fe_driver_setup_rx_ring(struct fe_device *dev, struct fe_driver_rx *rx, void *m,
                        uint64_t v2poff, uint16_t qlen)
{
    int ret;

    ret = -1;
    switch ( dev->driver ) {
    case FE_DRIVER_KERNEL:
        ret = -1;
        break;

    case FE_DRIVER_E1000:
        dev->rxq_last++;
        ret = e1000_setup_rx_ring(dev->u.e1000, &rx->u.e1000, dev->rxq_last, m,
                                  v2poff, qlen);
        break;

    case FE_DRIVER_IXGBE:
        dev->rxq_last++;
        ret = ixgbe_setup_rx_ring(dev->u.ixgbe, &rx->u.ixgbe, dev->rxq_last, m,
                                  v2poff, qlen);
        break;
    default:
        ret = -1;
    }

    return ret;
}

/*
 * Setup a Tx ring
 */
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
        dev->txq_last++;
        ret = e1000_setup_tx_ring(dev->u.e1000, &tx->u.e1000, dev->txq_last, m,
                                  v2poff, qlen);
        break;

    case FE_DRIVER_IXGBE:
        dev->txq_last++;
        ret = ixgbe_setup_tx_ring(dev->u.ixgbe, &tx->u.ixgbe, dev->txq_last, m,
                                  v2poff, qlen);
        break;

    default:
        ret = -1;
    }

    return ret;
}

/*
 * Calculate the required memory size for an Rx ring
 */
static __inline__ int
fe_driver_calc_rx_ring_memsize(struct fe_driver_rx *rx, uint16_t qlen)
{
    int ret;

    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        ret = (sizeof(struct fe_pkt_buf_hdr *) + sizeof(struct fe_kernel_desc))
            * qlen;
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

/*
 * Calculate the required memory size for a Tx ring
 */
static __inline__ int
fe_driver_calc_tx_ring_memsize(struct fe_driver_tx *tx, uint16_t qlen)
{
    int ret;

    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        ret = (sizeof(struct fe_pkt_buf_hdr *) + sizeof(struct fe_kernel_desc))
            * qlen;
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

/*
 * Refill Rx ring with a packet buffer from the buffer pool
 */
static __inline__ int
fe_driver_rx_refill(struct fe_task *t, struct fe_driver_rx *rx)
{
    struct fe_pkt_buf_hdr *pkt;
    void *pa;

    /* Try to get a packet buffer */
    pkt = fe_get_buffer(t);
    if ( NULL == pkt ) {
        return -1;
    }
    /* Resolve physical address */
    pa = fe_v2p(t, pkt);

    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        //fe_release_buffer(t, pkt);
        break;

    case FE_DRIVER_E1000:
        return e1000_rx_refill(&rx->u.e1000, pa + FE_PKT_HDROFF, pkt);

    case FE_DRIVER_IXGBE:
        return ixgbe_rx_refill(&rx->u.ixgbe, pa + FE_PKT_HDROFF, pkt);

    default:
        //fe_release_buffer(t, pkt);
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
        /* Always synchronized */
        break;

    case FE_DRIVER_E1000:
        e1000_rx_commit(&rx->u.e1000);
        break;

    case FE_DRIVER_IXGBE:
        ixgbe_rx_commit(&rx->u.ixgbe);
        break;

    default:
        ;
    }
}

static __inline__ int
fe_kernel_rx_dequeue(struct fe_kernel_ring *ring, struct fe_pkt_buf_hdr **hdr,
                     void **pkt)
{
    uint16_t head;
    int len;
    int port;

    if ( ring->rx_head == ring->tail ) {
        /* No more buffer available */
        return -1;
    }

    head = ring->rx_head + 1 < ring->len ? ring->rx_head + 1 : 0;
    *hdr = ring->bufs[ring->head];
    *pkt = ring->descs[ring->head].pkt;
    len = ring->descs[ring->head].length;
    port = ring->descs[ring->head].port;
    if ( 1 == ring->descs[ring->head].mode ) {
        *hdr = (void *)(uint64_t)port;
        len = 0;
    }
    __sync_synchronize();
    ring->rx_head = head;

    if ( len > 0 ) {
        (*hdr)->port = port;
    }

    return len;
}

static __inline__ int
fe_driver_rx_dequeue(struct fe_driver_rx *rx, struct fe_pkt_buf_hdr **hdr,
                     void **pkt)
{
    int ret;

    switch ( rx->driver ) {
    case FE_DRIVER_KERNEL:
        return fe_kernel_rx_dequeue(rx->u.kernel, hdr, pkt);

    case FE_DRIVER_E1000:
        ret = e1000_rx_dequeue(&rx->u.e1000, (void **)hdr);
        if ( ret > 0 ) {
            *pkt = (void *)*hdr + FE_PKT_HDROFF;
        }
        return ret;

    case FE_DRIVER_IXGBE:
        ret = ixgbe_rx_dequeue(&rx->u.ixgbe, (void **)hdr);
        if ( ret > 0 ) {
            *pkt = (void *)*hdr + FE_PKT_HDROFF;
        }
        return ret;

    default:
        ;
    }

    return -1;
}

static __inline__ int
fe_kernel_tx_enqueue(struct fe_kernel_ring *ring, int port, void *pkt,
                     void *hdr, size_t length)
{
    struct fe_kernel_desc *desc;
    uint16_t tail;

    tail = ring->tail + 1 < ring->len ? ring->tail + 1 : 0;
    if ( tail == ring->tx_head ) {
        /* Buffer is full */
        return 0;
    }
    desc = &ring->descs[ring->tail];
    desc->pkt = pkt;
    desc->length = length;
    desc->port = port;
    desc->mode = 0;
    ring->bufs[ring->tail] = hdr;
    ring->tail = tail;

    return 1;
}

static __inline__ int
fe_kernel_cmd_enqueue(struct fe_kernel_ring *ring, uint64_t mac, int port)
{
    struct fe_kernel_desc *desc;
    uint16_t tail;

    tail = ring->tail + 1 < ring->len ? ring->tail + 1 : 0;
    if ( tail == ring->tx_head ) {
        /* Buffer is full */
        return 0;
    }
    desc = &ring->descs[ring->tail];
    desc->pkt = (void *)mac;
    desc->length = 0;
    desc->port = port;
    desc->mode = 1;
    ring->bufs[ring->tail] = NULL;

    __sync_synchronize();

    ring->tail = tail;

    return 1;
}


static __inline__ int
fe_driver_tx_enqueue(struct fe_task *t, struct fe_driver_tx *tx, int port,
                     void *pkt, struct fe_pkt_buf_hdr *hdr, size_t length)
{
    int ret;

    switch ( tx->driver ) {
    case FE_DRIVER_KERNEL:
        ret = fe_kernel_tx_enqueue(tx->u.kernel, port, pkt, hdr, length);
        if ( ret > 0 ) {
            /* Increment the reference counter */
            hdr->refs++;
        }
        return ret;

    case FE_DRIVER_E1000:
        pkt = fe_v2p(t, pkt);
        ret = e1000_tx_enqueue(&tx->u.e1000, pkt, hdr, length);
        if ( ret > 0 ) {
            /* Increment the reference counter */
            hdr->refs++;
        }
        return ret;

    case FE_DRIVER_IXGBE:
        pkt = fe_v2p(t, pkt);
        ret = ixgbe_tx_enqueue(&tx->u.ixgbe, pkt, hdr, length);
        if ( ret > 0 ) {
            /* Increment the reference counter */
            hdr->refs++;
        }
        return ret;

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
        /* Do nothing */
        break;
    case FE_DRIVER_E1000:
        e1000_tx_commit(&tx->u.e1000);
        break;
    case FE_DRIVER_IXGBE:
        ixgbe_tx_commit(&tx->u.ixgbe);
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
