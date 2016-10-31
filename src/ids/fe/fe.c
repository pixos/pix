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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/pix.h>
#include <time.h>
#include "pci.h"
#include "fe.h"

/* Ethernet header */
struct ethhdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__ ((packed));

unsigned long long syscall(int, ...);

/*
 * Population count
 */
static __inline__ int
popcnt(uint64_t x)
{
    x = x - ((x>>1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x>>2) & 0x3333333333333333ULL);
    x = (x + (x>>4)) & 0x0f0f0f0f0f0f0f0fULL;
    x = x + (x>>8);
    x = x + (x>>16);
    x = x + (x>>32);

    return x & 0x7f;
}


/*
 * Is multicast MAC address?
 */
static __inline__ int
_is_muticast(uint8_t *addr)
{
    return addr[0] & 1;
}

/*
 * Forwarding (Fast-path)
 */
static int
fe_fpp_forwarding(struct fe_task *t, int port, struct fe_pkt_buf_hdr *hdr,
                  void *pkt, int len)
{
    struct ethhdr *eth;
    uint8_t key[FDB_KEY_SIZE];
    struct fdb_entry *e;
    ssize_t i;
    uint64_t mac;

    eth = (struct ethhdr *)pkt;

    memcpy(key, eth->dst, 6);
    memset(key + 6, 0, 2);
    e = fdb_lookup(t->fe->fdb, key);
    if ( NULL == e ) {
        /* No entry found, then flooding */
        for ( i = 0; i < (ssize_t)t->fe->nports; i++ ) {
            if ( port != i ) {
                fe_driver_tx_enqueue(t, &t->tx.rings[i], i, pkt, hdr, len);
            }
            fe_driver_tx_commit(&t->tx.rings[i]);
            fe_collect_buffer(t, &t->tx.rings[i]);
        }
    } else {
        /* Unicast */
        if ( e->port == port ) {
            /* Discard */
            fe_release_buffer(t, hdr);
        } else {
            fe_driver_tx_enqueue(t, &t->tx.rings[e->port], e->port, pkt, hdr,
                                 len);
            fe_driver_tx_commit(&t->tx.rings[e->port]);
            fe_collect_buffer(t, &t->tx.rings[e->port]);
        }
    }

    /* Check the source address to update FDB */
    if ( !_is_muticast(eth->src) ) {
        /* Unicast, then update the corresonding fdb entry */
        mac = 0;
        memcpy(&mac, eth->src, 6);
        fe_kernel_cmd_enqueue(t->ktx, mac, port);
    }

    return 0;
}

/*
 * Forwarding (Slow-path)
 */
static int
fe_spp_forwarding(struct fe_task *t, struct fe_driver_rx *rx,
                  struct fe_pkt_buf_hdr *hdr, void *pkt, int len)
{
    struct fe_pkt_buf_hdr *myhdr;
    void *mypkt;

    myhdr = fe_get_buffer(t);
    if ( NULL == myhdr ) {
        /* No buffer available */
        printf("Buffer empty\n");
        return -1;
    }
    /* Copy */
    memcpy(myhdr, hdr, FE_PKTSZ);
    mypkt = pkt - (void *)hdr + (void *)myhdr;
    /* Release */
    hdr->refs--;
    rx->u.kernel->head = rx->u.kernel->head + 1 < rx->u.kernel->len
        ? rx->u.kernel->head + 1 : 0;

    fe_driver_tx_enqueue(t, &t->tx.rings[myhdr->port], myhdr->port, mypkt, hdr,
                         len);
    fe_driver_tx_commit(&t->tx.rings[myhdr->port]);
    fe_collect_buffer(t, &t->tx.rings[myhdr->port]);

    return 0;
}





/*
 * Fast-path process
 */
void *
fe_fpp_task(void *args)
{
    struct fe_task *t;
    int ret;
    struct fe_pkt_buf_hdr *hdr;
    void *pkt;
    int n;
    int i;

    /* Get the task data structure from the argument */
    t = (struct fe_task *)args;

    /* Count the number of ports managed by this task */
    n = popcnt(t->rx.bitmap);

    printf("Launch an exclusive task for fast-path processing at CPU %d, "
           "managing %d ports.\n", t->cpuid, n);

    for ( ;; ) {
        for ( i = 0; i < n; i++ ) {
            ret = fe_driver_rx_dequeue(&t->rx.rings[i], &hdr, &pkt);
            if ( ret <= 0 ) {
                continue;
            }
            fe_driver_rx_refill(t, &t->rx.rings[i]);
            fe_fpp_forwarding(t, t->rx.rings[i].port, hdr, pkt, ret);

            fe_driver_rx_commit(&t->rx.rings[i]);
        }
        for ( i = 0; i < (ssize_t)t->fe->nports; i++ ) {
            fe_collect_buffer(t, &t->tx.rings[i]);
        }
    }
}

/*
 * Slow-path process
 */
int
fe_process(struct fe *fe)
{
    int i;
    int ret;
    struct fe_pkt_buf_hdr *hdr;
    void *pkt;
    uint64_t tsc;
    uint64_t last_tsc;

    last_tsc = 0;
    for ( ;; ) {
        /* For all exclusive processors */
        for ( i = 0; i < fe->nxcpu; i++ ) {
            ret = fe_driver_rx_dequeue(&fe->tftask->rx.rings[i], &hdr, &pkt);
            if ( ret < 0 ) {
                continue;
            }
            if ( 0 == ret ) {
                /* Command (non-packet) */
                fdb_update(fe->fdb, (uint8_t *)&pkt, (int)(uint64_t)hdr);
                fe->tftask->rx.rings[i].u.kernel->head
                    = fe->tftask->rx.rings[i].u.kernel->head + 1
                    < fe->tftask->rx.rings[i].u.kernel->len
                    ? fe->tftask->rx.rings[i].u.kernel->head + 1 : 0;
            } else {
                fe_driver_rx_refill(fe->tftask, &fe->tftask->rx.rings[i]);
                fe_spp_forwarding(fe->tftask, &fe->tftask->rx.rings[i], hdr,
                                  pkt, ret);
                fe_driver_rx_commit(&fe->tftask->rx.rings[i]);
            }
        }

        /* Garbage collection */
        tsc = fdb_rdtsc();
        if ( tsc - last_tsc > 10000000000ULL ) {
            fdb_gc(fe->fdb);
            /* Print out FDB */
            fdb_debug(fe->fdb);
            last_tsc = tsc;
        }
    }

    return 0;
}

/*
 * Create a job at the specified exclusive processor
 */
void
syspix_create_job(int cpuid, void *args)
{
    syscall(SYS_pix_create_job, cpuid, fe_fpp_task, args);
}

/*
 * Initialize device
 */
static struct fe_device *
_init_device(struct pci_dev_conf *conf)
{
    struct fe_device dev;
    struct fe_device *devp;

    /* Check the driver type and initialize the hardware */
    dev.driver = FE_DRIVER_INVALID;
    if ( e1000_is_e1000(conf->vendor_id, conf->device_id) ) {
        /* e1000 */
        dev.driver = FE_DRIVER_E1000;
        dev.u.e1000
            = e1000_init(conf->device_id, conf->bus, conf->slot, conf->func);
        e1000_init_hw(dev.u.e1000);
        e1000_setup_rx(dev.u.e1000);
        e1000_setup_tx(dev.u.e1000);
        dev.domain = 0;
        dev.rxq_last = -1;
        dev.txq_last = -1;
        dev.fastpath = 0;
    } else if ( ixgbe_is_ixgbe(conf->vendor_id, conf->device_id) ) {
        /* ixgbe */
        dev.driver = FE_DRIVER_IXGBE;
        dev.u.ixgbe
            = ixgbe_init(conf->device_id, conf->bus, conf->slot, conf->func);
        ixgbe_init_hw(dev.u.ixgbe);
        ixgbe_setup_rx(dev.u.ixgbe);
        ixgbe_setup_tx(dev.u.ixgbe);
        ixgbe_enable_rx(dev.u.ixgbe);
        ixgbe_enable_tx(dev.u.ixgbe);
        dev.domain = 0;
        dev.rxq_last = -1;
        dev.txq_last = -1;
        dev.fastpath = 0;
    }

    if ( FE_DRIVER_INVALID != dev.driver ) {
        devp = malloc(sizeof(struct fe_device));
        if ( NULL == devp ) {
            return NULL;
        }
        memcpy(devp, &dev, sizeof(struct fe_device));
        return devp;
    }

    return NULL;
}

/*
 * Initialize network devices
 */
int
fe_init_devices(struct fe *fe, struct pci_dev *pci)
{
    struct fe_device *dev;

    fe->nports = 0;
    while ( NULL != pci ) {
        dev = _init_device(pci->device);
        if ( NULL != dev ) {
            /* Device found */
            dev->port = fe->nports;
            fe->ports[fe->nports] = dev;
            fe->nports++;
        }
        pci = pci->next;
    }

    return 0;
}

/*
 * Initialize processors
 */
int
fe_init_cpu(struct fe *fe)
{
    struct syspix_cpu_table cputable;
    int n;
    int nex;
    ssize_t i;
    struct fe_task *t;
    struct fe_task **tp;

    /* Load the CPU table through system call */
    n = syscall(SYS_pix_cpu_table, SYSPIX_LDCTBL, &cputable);
    if ( n < 0 ) {
        return -1;
    }
    /* # of available CPUs */
    fe->ncpus = n;

    /* Tickful task */
    t = malloc(sizeof(struct fe_task));
    if ( NULL == t ) {
        return -1;
    }
    t->fe = fe;
    t->cpuid = -1;
    t->pool.head = NULL;
    t->pool.v2poff = 0;
    t->rx.bitmap = 0;
    t->rx.rings = NULL;
    t->tx.rings = NULL;
    t->ktx = NULL;
    t->next = NULL;

    /* Add */
    fe->tftask = t;
    fe->extasks = NULL;

    /* Find out the (number of) exclusive CPUs */
    nex = 0;
    for ( i = 0; i < PIX_MAX_CPU; i++ ) {
        if ( cputable.cpus[i].present ) {
            /* CPU is present */
            switch ( cputable.cpus[i].type ) {
            case SYSPIX_CPU_TICKFUL:
                /* Tickful (application) */
                break;
            case SYSPIX_CPU_EXCLUSIVE:
                /* Exclusive: Fast-path task */
                t = malloc(sizeof(struct fe_task));
                if ( NULL == t) {
                    return -1;
                }
                t->fe = fe;
                t->cpuid = i;
                t->pool.head = NULL;
                t->pool.v2poff = 0;
                t->rx.bitmap = 0;
                t->rx.rings = NULL;
                t->tx.rings = NULL;
                t->ktx = NULL;
                t->next = NULL;

                /* Append it to the tail */
                tp = &fe->extasks;
                while ( NULL != *tp ) {
                    tp = &(*tp)->next;
                }
                *tp = t;

                nex++;
                break;
            default:
                ;
            }
        }
    }

    /* # of exlusive CPUs */
    fe->nxcpu = nex;

    return 0;
}

/*
 * Initialize the buffer pool
 */
int
fe_init_buffer_pool(struct fe *fe)
{
    size_t len;
    void *pa;
    void *va;
    int ret;
    struct fe_task *t;
    uint64_t voff;
    void *pkt;
    ssize_t i;
    struct fe_pkt_buf_hdr *hdr;
    struct fe_pkt_buf_hdr *prev;

    /* Allocate packet buffer */
    len = (size_t)FE_PKTSZ * FE_BUFFER_POOL_SIZE * (fe->nxcpu + 1);
    ret = syscall(SYS_pix_malloc, len, &pa, &va);
    if ( ret < 0 ) {
        return -1;
    }
    voff = pa - va;

    /* Start from here */
    pkt = va;

    /* Tickful task */
    t = fe->tftask;
    /* Create a buffer pool for the tickful task */
    prev = NULL;
    for ( i = 0; i < FE_BUFFER_POOL_SIZE; i++ ) {
        hdr = (struct fe_pkt_buf_hdr *)pkt;
        hdr->next = prev;
        hdr->refs = 0;
        prev = hdr;
        pkt += FE_PKTSZ;
    }
    t->pool.head = hdr;
    t->pool.v2poff = voff;

    /* Exclusive CPUs */
    t = fe->extasks;
    while ( NULL != t ) {
        /* Create a buffer pool for each task */
        prev = NULL;
        for ( i = 0; i < FE_BUFFER_POOL_SIZE; i++ ) {
            hdr = (struct fe_pkt_buf_hdr *)pkt;
            hdr->next = prev;
            hdr->refs = 0;
            prev = hdr;
            pkt += FE_PKTSZ;
        }
        t->pool.head = hdr;
        t->pool.v2poff = voff;
        /* Next task */
        t = t->next;
    }

    return 0;
}

/*
 * Initialize the device type (fast-path or slow-path)
 */
int
fe_init_device_type(struct fe *fe)
{
    ssize_t i;
    int ntxq;

    for ( i = 0; i < (ssize_t)fe->nports; i++ ) {
        /* Get the maximum number of Tx queues */
        ntxq = fe_driver_max_tx_queues(fe->ports[i]);

        if ( fe->nxcpu + 1 > ntxq ) {
            /* Slow-path if the number of exclusive CPUs plus 1 (for kernel) is
               greater than the number of Tx queues */
            fe->ports[i]->fastpath = 0;
        } else {
            /* Fast-path */
            fe->ports[i]->fastpath = 1;
        }
    }

    return 0;
}

/*
 * Allocate
 */
static void *
_fe_alloc(struct fe *fe, size_t len)
{
    void *a;

    /* 64 byte alignment */
    len = (len + 128 - 1) / 128 * 128;

    a = fe->mem.free;
    if ( fe->mem.free + len > fe->mem.vaddr + fe->mem.len ) {
        return NULL;
    }
    fe->mem.free += len;

    return a;
}

/*
 * Initialize the ring buffers of am exclusive task
 */
static int
_init_extask_ring(struct fe *fe, struct fe_task *t, int n, int *port)
{
    ssize_t i;
    struct fe_kernel_ring *ring;
    int sz;
    void *m;
    int ret;

    /* Kernel Tx */
    t->ktx = _fe_alloc(fe, sizeof(struct fe_kernel_ring));
    if ( NULL == t->ktx ) {
        return -1;
    }
    ring = t->ktx;
    ring->len = FE_QLEN;
    ring->head = 0;
    ring->tail = 0;
    ring->rx_head = 0;
    ring->tx_head = 0;
    ring->descs = _fe_alloc(fe, sizeof(struct fe_kernel_desc) * ring->len);
    if ( NULL == ring->descs ) {
        return -1;
    }
    ring->bufs = _fe_alloc(fe, sizeof(struct fe_pkt_buf_hdr *) * ring->len);
    if ( NULL == ring->bufs ) {
        return -1;
    }

    /* Rx queues handled by this task */
    if ( (size_t)*port + n >= fe->nports ) {
        n = fe->nports - *port;
    }
    t->rx.rings = _fe_alloc(fe, sizeof(struct fe_driver_rx) * n);
    if ( NULL == t->rx.rings ) {
        return -1;
    }
    t->rx.bitmap = 0;
    for ( i = 0; i < n; i++ ) {
        t->rx.bitmap |= (1ULL << *port);
        /* Set driver */
        t->rx.rings[i].driver = fe->ports[*port]->driver;
        /* Set port # */
        t->rx.rings[i].port = *port;
        /* Calculate the required memory space */
        sz = fe_driver_calc_rx_ring_memsize(&t->rx.rings[i], FE_QLEN);
        if ( sz < 0 ) {
            return -1;
        }
        /* Allocate memory space for the ring */
        m = _fe_alloc(fe, sz);
        if ( NULL == m ) {
            return -1;
        }
        /* Setup an Rx queue */
        ret = fe_driver_setup_rx_ring(fe->ports[*port], &t->rx.rings[i], m,
                                      fe->mem.v2poff, FE_QLEN);
        if ( ret < 0 ) {
            return -1;
        }

        /* Fill the Rx queue */
        fe_driver_rx_fill_all(t, &t->rx.rings[i]);
        fe_driver_rx_commit(&t->rx.rings[i]);

        /* Next port */
        (*port)++;
    }

    /* Tx */
    t->tx.rings = _fe_alloc(fe, sizeof(struct fe_driver_tx) * fe->nports);
    if ( NULL == t->tx.rings ) {
        return -1;
    }

    /* Physical ports */
    for ( i = 0; i < (ssize_t)fe->nports; i++ ) {
        if ( fe->ports[i]->fastpath ) {
            /* Fast-path */
            t->tx.rings[i].driver = fe->ports[i]->driver;

            sz = fe_driver_calc_tx_ring_memsize(&t->tx.rings[i], FE_QLEN);
            if ( sz < 0 ) {
                return -1;
            }
            m = _fe_alloc(fe, sz);
            if ( NULL == m ) {
                return -1;
            }
            ret = fe_driver_setup_tx_ring(fe->ports[i], &t->tx.rings[i], m,
                                          fe->mem.v2poff, FE_QLEN);
            if ( ret < 0 ) {
                return -1;
            }
        } else {
            /* Slow-path */
            t->tx.rings[i].driver = FE_DRIVER_KERNEL;
            t->tx.rings[i].u.kernel = t->ktx;
        }
    }

    return 0;
}


/*
 * Assign each device to a task
 */
int
fe_assign_task(struct fe *fe)
{
    struct fe_task *t;
    int n;
    int ret;
    int port;
    ssize_t i;
    int sz;
    void *m;

    /* # of ports (Rx queues) per task of an exclusive processor */
    n = ((fe->nports - 1) / fe->nxcpu) + 1;
    port = 0;

    /* Assign ports to each exclusive task */
    t = fe->extasks;
    while ( NULL != t ) {
        ret = _init_extask_ring(fe, t, n, &port);
        if ( ret < 0 ) {
            return -1;
        }

        /* Next task */
        t = t->next;
    }

    /* Tickful task */
    /* Rx from exclusive processors */
    fe->tftask->rx.bitmap = (1ULL << fe->nxcpu) - 1;
    fe->tftask->rx.rings
        = _fe_alloc(fe, sizeof(struct fe_driver_rx) * fe->nxcpu);
    if ( NULL == fe->tftask->rx.rings ) {
        return -1;
    }
    t = fe->extasks;
    i = 0;
    while ( NULL != t ) {
        fe->tftask->rx.rings[i].driver = FE_DRIVER_KERNEL;
        fe->tftask->rx.rings[i].u.kernel = t->ktx;
        /* Next task */
        t = t->next;
        i++;
    }

    /* Tx */
    fe->tftask->tx.rings
        = _fe_alloc(fe, sizeof(struct fe_driver_tx) * fe->nports);
    if ( NULL == fe->tftask->tx.rings ) {
        return -1;
    }
    /* Physical ports */
    for ( i = 0; i < (ssize_t)fe->nports; i++ ) {
        fe->tftask->tx.rings[i].driver = fe->ports[i]->driver;

        sz = fe_driver_calc_tx_ring_memsize(&fe->tftask->tx.rings[i], FE_QLEN);
        if ( sz < 0 ) {
            return -1;
        }
        m = _fe_alloc(fe, sz);
        if ( NULL == m ) {
            return -1;
        }
        ret = fe_driver_setup_tx_ring(fe->ports[i], &fe->tftask->tx.rings[i], m,
                                      fe->mem.v2poff, FE_QLEN);
        if ( ret < 0 ) {
            return -1;
        }
    }

    return 0;
}

/*
 * Initialize the forwarding engine
 */
int
fe_init(struct fe *fe)
{
    struct pci_dev *pci;
    int ret;
    size_t len;
    void *pa;
    void *va;
    uint64_t voff;

    /* Check all PCI devices */
    pci = pci_init();
    if ( NULL == pci ) {
        return -1;
    }

    /* Reset */
    fe->ncpus = 0;
    fe->nxcpu = 0;
    fe->nports = 0;
    memset(fe->ports, 0, sizeof(struct fe_device *) * FE_MAX_PORTS);
    fe->tftask = NULL;
    fe->extasks = NULL;

    /* Initialize the forwarding database */
    fe->fdb = fdb_init();
    if ( NULL == fe->fdb ) {
        printf("Failed to initilize FDB.\n");
        return -1;
    }

    /* Allocate memory for descriptors */
    len = (size_t)FE_MEMSIZE_FOR_DESCS;
    ret = syscall(SYS_pix_malloc, len, &pa, &va);
    if ( ret < 0 ) {
        printf("Failed to allocate memory.\n");
        return -1;
    }
    voff = pa - va;
    fe->mem.vaddr = va;
    fe->mem.len = len;
    fe->mem.v2poff = voff;
    fe->mem.free = va;

    /* Initialize processor (as a list of tasks) */
    ret = fe_init_cpu(fe);
    if ( ret < 0 ) {
        printf("Failed to initialize CPU.\n");
        goto error;
    }

    /* Initialize buffer pool */
    ret = fe_init_buffer_pool(fe);
    if ( ret < 0 ) {
        printf("Failed to initialize buffer pool.\n");
        goto error;
    }

    /* Initialize devices (hw) */
    ret = fe_init_devices(fe, pci);
    if ( ret < 0 ) {
        printf("Failed to initialize network devices.\n");
        goto error;
    }

    /* Release PCI memory */
    pci_release(pci);

    /* Check the number of exclusive CPUs and the number of ports whether each
       port supports fast-path */
    ret = fe_init_device_type(fe);
    if ( ret < 0 ) {
        return -1;
    }

    /* Assign task */
    ret = fe_assign_task(fe);
    if ( ret < 0 ) {
        printf("Failed to initialize tasks.\n");
        return -1;
    }

    return 0;
error:
    /* Release PCI memory */
    pci_release(pci);

    return -1;
}

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    struct timespec tm;
    struct fe fe;
    int fd[3];
    int ret;
    struct fe_task *t;

    /* /dev/console for stdin/stdout/stderr */
    fd[0] = open("/dev/console", O_RDWR);
    fd[1] = open("/dev/console", O_RDWR);
    fd[2] = open("/dev/console", O_RDWR);
    (void)fd[0];

    /* Initialize the forwarding engine */
    ret = fe_init(&fe);
    if ( ret < 0 ) {
        fprintf(stderr, "Failed to initialize the forwarding engine.\n");
        return EXIT_FAILURE;
    }

    /* Run threads */
    t = fe.extasks;
    while ( NULL != t ) {
        syspix_create_job(t->cpuid, (void *)t);
        /* Next task */
        t = t->next;
    }

    /* Tickful task */
    fe_process(&fe);

    tm.tv_sec = 1;
    tm.tv_nsec = 0;
    while ( 1 ) {
        nanosleep(&tm, NULL);
    }

    exit(0);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
