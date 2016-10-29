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
    uint64_t dst:48;
    uint64_t src:48;
    uint16_t type;
} __attribute__ ((packed));
/* IEEE 802.1Q */
struct ethhdr1q {
    uint16_t vlan;
    uint16_t type;
} __attribute__ ((packed));

/* IP header */
struct iphdr {
    uint8_t ip_ihl:4;           /* Little endian */
    uint8_t ip_version:4;
    uint8_t ip_tos;
    uint16_t ip_len;
    uint16_t ip_id;
    uint16_t ip_off;
    uint16_t ip_ttl;
    uint8_t ip_proto;
    uint16_t ip_sum;
    uint32_t ip_src;
    uint32_t ip_dst;
} __attribute__ ((packed));

/* ICMP header */
struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t ident;
    uint16_t seq;
    // data...
} __attribute__ ((packed));

/* IPv6 header */
struct ip6hdr {
    uint32_t ip6_vtf;
    uint16_t ip6_len;
    uint8_t ip6_next;
    uint8_t ip6_limit;
    uint8_t ip6_src[16];
    uint8_t ip6_dst[16];
} __attribute__ ((packed));

/* ICMPv6 header */
struct icmp6_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    // data...
} __attribute__ ((packed));

/* ARP */
struct ip_arp {
    uint16_t hw_type;
    uint16_t protocol;
    uint8_t hlen;
    uint8_t plen;
    uint16_t opcode;
    uint64_t src_mac:48;
    uint32_t src_ip;
    uint64_t dst_mac:48;
    uint32_t dst_ip;
} __attribute__ ((packed));


unsigned long long syscall(int, ...);

/*
 * Fast-path process
 */
void *
fe_fpp_task(void *args)
{
    for ( ;; ) {
        printf("%p", args);

        /* Do nothing */
        syscall(SYS_xpsleep);
    }
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
        dev.domain = 0;
        dev.fastpath = 0;
    } else if ( ixgbe_is_ixgbe(conf->vendor_id, conf->device_id) ) {
        /* ixgbe */
        dev.driver = FE_DRIVER_IXGBE;
        dev.u.ixgbe
            = ixgbe_init(conf->device_id, conf->bus, conf->slot, conf->func);
        ixgbe_init_hw(dev.u.ixgbe);
        dev.domain = 0;
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

    /* Load the CPU table through system call */
    n = syscall(SYS_pix_cpu_table, SYSPIX_LDCTBL, &cputable);
    if ( n < 0 ) {
        return -1;
    }
    /* # of available CPUs */
    fe->ncpus = n;

    /* Tickfull task */
    t = malloc(sizeof(struct fe_task));
    if ( NULL == t) {
        return -1;
    }
    t->cpuid = -1;
    t->next = NULL;

    /* Add */
    fe->tasks = t;

    /* Find out the (number of) exclusive CPUs */
    nex = 0;
    for ( i = 0; i < PIX_MAX_CPU; i++ ) {
        if ( cputable.cpus[i].present ) {
            /* CPU is present */
            switch ( cputable.cpus[i].type ) {
            case SYSPIX_CPU_TICKFULL:
                /* Tickfull (application) */
                break;
            case SYSPIX_CPU_EXCLUSIVE:
                /* Exclusive: Fast-path task */
                t = malloc(sizeof(struct fe_task));
                if ( NULL == t) {
                    return -1;
                }
                t->cpuid = i;
                t->next = fe->tasks;
                fe->tasks = t;

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

    t = fe->tasks;
    pkt = va;
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
 * Slow-path process
 */
int
fe_process(struct fe *fe)
{
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

    /* Check all PCI devices */
    pci = pci_init();
    if ( NULL == pci ) {
        return -1;
    }

    /* Initialize processor (as a list of tasks) */
    ret = fe_init_cpu(fe);
    if ( ret < 0 ) {
        goto error;
    }

    /* Initialize devices */
    ret = fe_init_devices(fe, pci);
    if ( ret < 0 ) {
        goto error;
    }

    /* Release PCI memory */
    pci_release(pci);

    /* Initialize buffer pool */
    ret = fe_init_buffer_pool(fe);
    if ( ret < 0 ) {
        return -1;
    }

    /* Check the number of exclusive CPUs and the number of ports whether each
       port supports fast-path */

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

    /* /dev/console for stdin/stdout/stderr */
    fd[0] = open("/dev/console", O_RDWR);
    fd[1] = open("/dev/console", O_RDWR);
    fd[2] = open("/dev/console", O_RDWR);
    (void)fd[0];

    /* Initialize the forwarding engine */
    ret = fe_init(&fe);
    if ( ret < 0 ) {
        return EXIT_FAILURE;
    }

    /* Run threads */
    syspix_create_job(1, (void *)0x1234);

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
