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

#define FE_PKTSZ                (10240 + 128)
#define FE_PKT_HDROFF           512
#define FE_BUFFER_POOL_SIZE     8192

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
    struct fe_pkt_buf_hdr *pkts;
    volatile uint32_t head;
    volatile uint32_t tail;
};

/*
 * Physical port
 */
struct fe_port {
    /* MMIO base address */
    uint64_t mmio;
    /* NUMA domain */
    int domain;
    /* Mac address */
    uint8_t mac[6];
    /* Driver specific data structure */
    struct {
        uint16_t type;
        union {
            struct e1000_device *e1000;
            struct ixgbe_device *ixgbe;
        } device;
    } driver;
    /* Rx */
    void *rx_descs;
};

/*
 * Driver
 */
struct fe_driver_tx {
    /* Driver type */
    uint16_t driver;
    union {
        struct fe_kernel_ring *kernel;
        struct e1000_tx_ring *e1000;
        struct ixgbe_tx_ring *ixgbe;
    } u;
};
struct fe_driver_rx {
    /* Driver type */
    uint16_t driver;
    union {
        struct fe_kernel_ring *kernel;
        struct e1000_rx_ring *e1000;
        struct ixgbe_rx_ring *ixgbe;
    } u;
};

/*
 * Data per processor
 */
struct fe_cpu {
    /* Buffer pool */
    struct fe_buffer_pool pool;

    /* Handling Rx queues */
    struct {
        uint64_t bitmap;
        struct fe_driver_rx *rings;
    } rx;

    /* Handling Tx queues */
    struct {
        uint64_t bitmap;
        struct fe_driver_tx *rings;
    } tx;
};

/*
 * Physical port
 */
struct fe_phys_port {
    /* NUMA domain */
    int domain;
    /* Driver */
    int driver;
    /* Device data */
    union {
        struct e1000_device *e1000;
        struct ixgbe_device *ixgbe;
    } u;
};

/*
 * Device
 */
struct fe_devices {
    size_t n;
    struct fe_phys_port *ports;
};

/*
 * Forwarding engine
 */
struct fe {
    /* Forwarding/Action database */
    void *fdb;

    /* Exclusive processors */
    int nxcpu;

    /* Processors */
    size_t ncpus;
    struct fe_cpu *cpus;
};


struct fe_config_port {
    uint16_t bus;
    uint16_t slot;
    uint16_t func;
};

static struct fe_config_port fe_ports[] = {
    { .bus = 0, .slot = 0x3, .func = 0 },
    { .bus = 0, .slot = 0x8, .func = 0 },
    { .bus = 0, .slot = 0x9, .func = 0 },
    { .bus = 0, .slot = 0xa, .func = 0 },
};

#endif /* _FE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
