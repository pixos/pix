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

#ifndef _IXGBE_H
#define _IXGBE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <mki/driver.h>
#include "pci.h"
#include "common.h"

#define IXGBE_X520DA2           0x10fb
#define IXGBE_X520QDA1          0x1558

#define IXGBE_MMIO_SIZE         0x12000

#define IXGBE_REG_RAL(n)        0xa200 + 8 * (n)
#define IXGBE_REG_RAH(n)        0xa204 + 8 * (n)
#define IXGBE_REG_CTRL          0x0000
#define IXGBE_REG_STATUS        0x0008
#define IXGBE_REG_CTRL_EXT      0x0018
#define IXGBE_REG_EICR          0x0800
#define IXGBE_REG_EIMC          0x0888
#define IXGBE_REG_MTA(n)        (0x5200 + (n) * 4)  /* x128 */
#define IXGBE_REG_RDRXCTL       0x2f00
#define IXGBE_REG_RXCTL         0x3000
#define IXGBE_REG_FCTRL         0x5080
#define IXGBE_REG_FCTTV(n)      (0x3200 + 4 * (n))
#define IXGBE_REG_FCRTL(n)      (0x3220 + 4 * (n))
#define IXGBE_REG_FCRTH(n)      (0x3260 + 4 * (n))
#define IXGBE_REG_FCRTV         0x32a0
#define IXGBE_REG_FCCFG         0x3d00
#define IXGBE_REG_TXDCTL(n)     (0x6028 + 0x40 * (n))
#define IXGBE_REG_RDBAL(n)       ((n) < 64) \
    ? (0x1000 + 0x40 * (n)) : (0xd000 + 0x40 * ((n) - 64))
#define IXGBE_REG_RDBAH(n)       ((n) < 64) \
    ? (0x1004 + 0x40 * (n)) : (0xd004 + 0x40 * ((n) - 64))
#define IXGBE_REG_RDLEN(n)       ((n) < 64) \
    ? (0x1008 + 0x40 * (n)) : (0xd008 + 0x40 * ((n) - 64))
#define IXGBE_REG_RDH(n)        ((n) < 64) \
    ? (0x1010 + 0x40 * (n)) : (0xd010 + 0x40 * ((n) - 64))
#define IXGBE_REG_RDT(n)        ((n) < 64) \
    ? (0x1018 + 0x40 * (n)) : (0xd018 + 0x40 * ((n) - 64))
#define IXGBE_REG_RXDCTL(n)     ((n) < 64) \
    ? (0x1028 + 0x40 * (n)) : (0xd028 + 0x40 * ((n) - 64))
#define IXGBE_REG_SRRCTL(n)     ((n) < 64) \
    ? (0x1014 + 0x40 * (n)) : (0xd014 + 0x40 * ((n) - 64))
#define IXGBE_REG_RSCCTL(n)     ((n) < 64) \
    ? (0x102c + 0x40 * (n)) : (0xd02c + 0x40 * ((n) - 64))
#define IXGBE_REG_TDH(n)        (0x6010 + 0x40 * (n))
#define IXGBE_REG_TDT(n)        (0x6018 + 0x40 * (n))
#define IXGBE_REG_TDBAL(n)      (0x6000 + 0x40 * (n))
#define IXGBE_REG_TDBAH(n)      (0x6004 + 0x40 * (n))
#define IXGBE_REG_TDLEN(n)      (0x6008 + 0x40 * (n))
#define IXGBE_REG_TDWBAL(n)     (0x6038 + 0x40 * (n))
#define IXGBE_REG_TDWBAH(n)     (0x603c + 0x40 * (n))
#define IXGBE_REG_HLREG0        0x04240
#define IXGBE_REG_DMATXCTL      0x4a80
#define IXGBE_REG_GCR_EXT       0x11050

#define IXGBE_REG_EEC           0x10010
#define IXGBE_REG_AUTOC         0x42a0
#define IXGBE_REG_AUTOC2        0x42a8
#define IXGBE_REG_MSCA          0x425c
#define IXGBE_REG_MSRWD         0x4260

#define IXGBE_REG_LINKS         0x42a4
#define IXGBE_REG_LINKS2        0x4324

#define IXGBE_REG_MFLCN         0x4294
#define IXGBE_REG_TFCS          0xce00
#define IXGBE_REG_MNGTXMAP      0xcd10

#define IXGBE_REG_SECRXCTRL     0x8d00
#define IXGBE_REG_SECRXSTAT     0x8d04

#define IXGBE_REG_FDIRCMD       0xee2c
#define IXGBE_REG_FDIRFREE      0xee38
#define IXGBE_REG_FDIRHASH      0xee28

#define IXGBE_REG_MCSTCTRL      0x5090

#define IXGBE_REG_VFTA(n)       (0xa000 + 0x4 * (n))

/* RSS */
#define IXGBE_REG_RETA(n)       (0x05c00 + 4 * (n))
#define IXGBE_REG_MRQC          0x05818
/* [3:0] = 0001b for RSS: [17] = IPv4, [20] = IPv6 */

/* DCA registers */
#define IXGBE_REG_DCA_RXCTRL(n) ((n) < 64) \
    ? (0x100c + 0x40 * (n)) : (0xd00c + 0x40 * ((n) - 64))
#define IXGBE_REG_DCA_TXCTRL(n) (0x600c + 0x40 * (n))
#define IXGBE_REG_DCA_ID        0x11070
#define IXGBE_REG_DCA_CTRL      0x11074

#define IXGBE_REG_MAXFRS        0x04268

#define IXGBE_CTRL_LRST (1<<3)  /* Link reset */
#define IXGBE_CTRL_PCIE_MASTER_DISABLE  (uint32_t)(1<<2)
#define IXGBE_CTRL_RST  (1<<26)

#define IXGBE_STATUS_PCIE_MASTER_ENABLE  (uint32_t)(1<<19)

#define IXGBE_FCTRL_SBP         (1<<1)
#define IXGBE_FCTRL_MPE         (1<<8)
#define IXGBE_FCTRL_UPE         (1<<9)
#define IXGBE_FCTRL_BAM         (1<<10)

#define IXGBE_SRRCTL_BSIZE_PKT2K        (2)
#define IXGBE_SRRCTL_BSIZE_PKT4K        (4)
#define IXGBE_SRRCTL_BSIZE_PKT8K        (8)
#define IXGBE_SRRCTL_BSIZE_PKT10K       (10)
#define IXGBE_SRRCTL_BSIZE_PKT16K       (16)
#define IXGBE_SRRCTL_BSIZE_HDR256       (4<<8)
#define IXGBE_SRRCTL_DESCTYPE_LEGACY    (0)

#define IXGBE_RXDCTL_ENABLE     (1<<25)
#define IXGBE_RXDCTL_VME        (1<<30)
#define IXGBE_RXCTL_RXEN        1
#define IXGBE_TXDCTL_ENABLE     (1<<25)
#define IXGBE_DMATXCTL_TE       1
#define IXGBE_DMATXCTL_VT       0x8100

#define IXGBE_HLREG0_TXCRCEN    1
#define IXGBE_HLREG0_RXCRCSTRP  (1 << 1)
#define IXGBE_HLREG0_JUMBOEN    (1 << 2)
#define IXGBE_HLREG0_TXPADEN    (1 << 10)
#define IXGBE_HLREG0_LPBK       (1 << 15)
#define IXGBE_HLREG0_RXLNGTHERREN       (1 << 27)
#define IXGBE_HLREG0_RXPADSTRIPEN       (1 << 28)

#define IXGBE_RDRXCTL_DMAIDONE  (1 << 3)

#define IXGBE_EEC_AUTO_RD       (1 << 9)


/* # of rings supported by 82599 */
#define IXGBE_NRXQ              32
#define IXGBE_NTXQ              256     /* 32 for 82598, 256 for 82599 */

/*
 * Receive descriptor
 */
struct ixgbe_rx_desc_read {
    uint64_t pkt_addr;          /* Bit 0: A0 */
    volatile uint64_t hdr_addr; /* Bit 0: DD */
} __attribute__ ((packed));
struct ixgbe_rx_desc_wb {
    uint32_t info0;
    uint32_t info1;
    uint32_t staterr;
    uint16_t length;
    uint16_t vlan;
} __attribute__ ((packed));
union ixgbe_rx_desc {
    struct ixgbe_rx_desc_read read;
    struct ixgbe_rx_desc_wb wb;
} __attribute__ ((packed));

/*
 * Transmit descriptor
 */
struct ixgbe_tx_desc_ctx {
    uint32_t vlan_maclen_iplen;
    uint32_t fcoef_ipsec_sa_idx;
    uint64_t other;
} __attribute__ ((packed));
struct ixgbe_tx_desc_data {
    uint64_t pkt_addr;
    uint16_t length;
    uint8_t dtyp_mac;
    uint8_t dcmd;
    uint32_t paylen_popts_cc_idx_sta;
} __attribute__ ((packed));
union ixgbe_tx_desc {
    struct ixgbe_tx_desc_ctx ctx;
    struct ixgbe_tx_desc_data data;
} __attribute__ ((packed));

/*
 * Rx ring buffer
 */
struct ixgbe_rx_ring {
    union ixgbe_rx_desc *descs;
    void **bufs;
    uint16_t tail;
    uint16_t head;
    uint16_t len;
    /* Queue information */
    uint16_t idx;               /* Queue index */
    void *mmio;                 /* MMIO */
};

/*
 * Tx ring buffer
 */
struct ixgbe_tx_ring {
    union ixgbe_tx_desc *descs;
    void **bufs;
    uint16_t tail;
    uint16_t head;
    uint16_t len;
    /* Write-back */
    uint32_t *tdwba;
    /* Queue information */
    uint16_t idx;               /* Queue index */
    void *mmio;                 /* MMIO */
};

/*
 * ixgbe device
 */
struct ixgbe_device {
    void *mmio;
    uint8_t macaddr[6];
    uint16_t device_id;
};


/*
 * Prototype declarations
 */
static __inline__ int ixgbe_read_mac_address(struct ixgbe_device *);


/*
 * Check if the device is ixgbe
 */
static __inline__ int
ixgbe_is_ixgbe(uint16_t vendor_id, uint16_t device_id)
{
    /* Must be Intel */
    if ( 0x8086 != vendor_id ) {
        return 0;
    }
    switch ( device_id ) {
    case IXGBE_X520DA2:
    case IXGBE_X520QDA1:
        return 1;
    default:
        return 0;
    }
}

/*
 * Initialize ixgbe device
 */
static __inline__ struct ixgbe_device *
ixgbe_init(uint16_t device_id, uint16_t bus, uint16_t slot, uint16_t func)
{
    struct ixgbe_device *dev;
    uint64_t pmmio;
    uint32_t m32;

    /* Allocate an ixgbe device data structure */
    dev = malloc(sizeof(struct ixgbe_device));
    if ( NULL == dev ) {
        return NULL;
    }
    dev->device_id = device_id;

    /* Read MMIO */
    pmmio = pci_read_mmio(bus, slot, func);
    dev->mmio = driver_mmap((void *)pmmio, IXGBE_MMIO_SIZE);
    if ( NULL == dev->mmio ) {
        /* Error */
        free(dev);
        return NULL;
    }

    /* Initialize the PCI configuration space */
    m32 = pci_read_config(bus, slot, func, 0x4);
    pci_write_config(bus, slot, func, 0x4, m32 | 0x7);

    /* Get the device MAC address */
    ixgbe_read_mac_address(dev);

    return dev;
}

/*
 * The number of supported Tx queues
 */
static __inline__ int
ixgbe_max_tx_queues(struct ixgbe_device *dev)
{
    (void)dev;
    /* For 82599 */
    return 128;
}

/*
 * Get the device MAC address
 */
static __inline__ int
ixgbe_read_mac_address(struct ixgbe_device *dev)
{
    uint32_t m32;

    /* Read MAC address */
    m32 = rd32(dev->mmio, IXGBE_REG_RAL(0));
    dev->macaddr[0] = m32 & 0xff;
    dev->macaddr[1] = (m32 >> 8) & 0xff;
    dev->macaddr[2] = (m32 >> 16) & 0xff;
    dev->macaddr[3] = (m32 >> 24) & 0xff;
    m32 = rd32(dev->mmio, IXGBE_REG_RAH(0));
    dev->macaddr[4] = m32 & 0xff;
    dev->macaddr[5] = (m32 >> 8) & 0xff;

    return 0;
}

/*
 * Initialize the hardware
 */
static __inline__ int
ixgbe_init_hw(struct ixgbe_device *dev)
{
    ssize_t i;
    struct timespec tm;
    uint32_t m32;

    /* Initialization sequence: S4.6.3 */

    /* 1. Disable interrupts */
    wr32(dev->mmio, IXGBE_REG_EIMC, 0x7fffffff);
    /* Clear any pending interrupts */
    rd32(dev->mmio, IXGBE_REG_EICR);

    /* 2. Issue global reset and perform general configuration (S4.6.3.2) */
    for ( i = 0; i < 4; i++ ) {
        wr32(dev->mmio, IXGBE_REG_FCTTV(i), 0);
    }
    for ( i = 0; i < 8; i++ ) {
        wr32(dev->mmio, IXGBE_REG_FCRTL(i), 0);
        wr32(dev->mmio, IXGBE_REG_FCRTH(i), 0);
    }
    wr32(dev->mmio, IXGBE_REG_FCRTV, 0);
    wr32(dev->mmio, IXGBE_REG_FCCFG, 0);

    /* Disable Rx and clear */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, IXGBE_REG_RXDCTL(i), 0);
        wr32(dev->mmio, IXGBE_REG_RDH(i), 0);
        wr32(dev->mmio, IXGBE_REG_RDT(i), 0);
    }
    wr32(dev->mmio, IXGBE_REG_RXCTL,
         rd32(dev->mmio, IXGBE_REG_RXCTL) & ~IXGBE_RXCTL_RXEN);
    (void)rd32(dev->mmio, IXGBE_REG_STATUS);
    /* Sleep 2 ms */
    tm.tv_sec = 0;
    tm.tv_nsec = 2000000;
    nanosleep(&tm, NULL);
    wr32(dev->mmio, IXGBE_REG_HLREG0,
         rd32(dev->mmio, IXGBE_REG_HLREG0) | (1 << 15)); /* LPBK */
    wr32(dev->mmio, IXGBE_REG_GCR_EXT,     /* Buffers_Clear_Func */
         rd32(dev->mmio, IXGBE_REG_GCR_EXT) | (1 << 30));
    /* Sleep 20 us */
    tm.tv_sec = 0;
    tm.tv_nsec = 20000;
    nanosleep(&tm, NULL);
    wr32(dev->mmio, IXGBE_REG_HLREG0,
         rd32(dev->mmio, IXGBE_REG_HLREG0) & ~(1 << 15));
    wr32(dev->mmio, IXGBE_REG_GCR_EXT,
         rd32(dev->mmio, IXGBE_REG_GCR_EXT) & ~(1 << 30));
    /* Disable all Tx queues */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, IXGBE_REG_TXDCTL(i),
             rd32(dev->mmio, IXGBE_REG_TXDCTL(i)) & ~IXGBE_TXDCTL_ENABLE);
    }

    /* Disable PCIe master */
    wr32(dev->mmio, IXGBE_REG_CTRL,
         rd32(dev->mmio, IXGBE_REG_CTRL) | IXGBE_CTRL_PCIE_MASTER_DISABLE);
    for ( i = 0; i < 128; i++ ) {
        /* Sleep 1 ms */
        tm.tv_sec = 0;
        tm.tv_nsec = 1000000;
        nanosleep(&tm, NULL);
        m32 = rd32(dev->mmio, IXGBE_REG_STATUS);
        if ( !(m32 & IXGBE_STATUS_PCIE_MASTER_ENABLE) ) {
            break;
        }
    }
    if ( m32 & IXGBE_STATUS_PCIE_MASTER_ENABLE ) {
        printf("Error on disabling PCIe master %x\n", m32);
        //return -1;
    }

    /* Issue a global reset (a.k.a. software reset) */
    wr32(dev->mmio, IXGBE_REG_CTRL,
         rd32(dev->mmio, IXGBE_REG_CTRL) | IXGBE_CTRL_RST);
    for ( i = 0; i < 128; i++ ) {
        /* Sleep 1 ms */
        tm.tv_sec = 0;
        tm.tv_nsec = 1000000;
        nanosleep(&tm, NULL);
        m32 = rd32(dev->mmio, IXGBE_REG_CTRL);
        if ( !(m32 & IXGBE_CTRL_RST) ) {
            break;
        }
    }
    if ( m32 & IXGBE_CTRL_RST ) {
        printf("Error on reset %p %d\n", dev->mmio, m32);
        return -1;
    }

    /* Set PFRSTD */
    wr32(dev->mmio, IXGBE_REG_CTRL_EXT,
         rd32(dev->mmio, IXGBE_REG_CTRL_EXT) | (1 << 14));

    /* Sleep 50 ms */
    tm.tv_sec = 0;
    tm.tv_nsec = 50000000;
    nanosleep(&tm, NULL);

    /* 3. Wait for EEPROM auto read completion */
    for ( i = 0; i < 1024; i++ ) {
        /* Sleep 10 us */
        tm.tv_sec = 0;
        tm.tv_nsec = 10000;
        nanosleep(&tm, NULL);

        m32 = rd32(dev->mmio, IXGBE_REG_EEC);
        if ( m32 & IXGBE_EEC_AUTO_RD ) {
            /* EEPROM Auto-Read Done */
            break;
        }
    }
    if ( !(m32 & IXGBE_EEC_AUTO_RD) ) {
        printf("Error on EEPROM read\n");
        return -1;
    }

    /* 4. Wait for DMA initialization done (RDRXCTL.DMAIDONE) */
    for ( i = 0; i < 1024; i++ ) {
        /* Sleep 10 us */
        tm.tv_sec = 0;
        tm.tv_nsec = 10000;
        nanosleep(&tm, NULL);

        m32 = rd32(dev->mmio, IXGBE_REG_RDRXCTL);
        if ( m32 & IXGBE_RDRXCTL_DMAIDONE ) {
            /* DMA initialization done */
            break;
        }
    }
    if ( !(m32 & IXGBE_RDRXCTL_DMAIDONE) ) {
        printf("Error on DMA initialization\n");
        return -1;
    }

    /* 5. Setup the PHY and the link (S4.6.4) */

    /* 6. Initialize all statistical counters (S4.6.5) */

    /* Multicast array table */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, IXGBE_REG_MTA(i), 0);
    }

    /* 7. Initialize receive (S4.6.7) */
    /* 8. Initialize transmit (S4.6.8) */
    /* 9. Enable interrupts (S4.6.3.1) */

    return 0;
}

/*
 * Setup Rx port
 */
static __inline__ int
ixgbe_setup_rx(struct ixgbe_device *dev)
{
    ssize_t i;

    /* Support jumbo frame (0x2400 = 9216 bytes) */
    wr32(dev->mmio, IXGBE_REG_MAXFRS, 0x2400 << 16);
    wr32(dev->mmio, IXGBE_REG_HLREG0,
         rd32(dev->mmio, IXGBE_REG_HLREG0) | (1 << 2));

    wr32(dev->mmio, IXGBE_REG_FCTRL,
         IXGBE_FCTRL_MPE | IXGBE_FCTRL_UPE | IXGBE_FCTRL_BAM);

    /* CRC strip */
    wr32(dev->mmio, IXGBE_REG_RDRXCTL,
         (rd32(dev->mmio, IXGBE_REG_RDRXCTL) | (1 << 1)) & ~(0x3e0000));

    /* NS_DIS */
    wr32(dev->mmio, IXGBE_REG_CTRL_EXT,
         rd32(dev->mmio, IXGBE_REG_CTRL_EXT) | (1 << 16));

    /* Clear VLAN filter */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, IXGBE_REG_VFTA(i), 0);
    }

    /* Clear RSC */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, IXGBE_REG_RSCCTL(i), 0);
   }

    /* Clear multi queue */
    wr32(dev->mmio, IXGBE_REG_MRQC, 0);

    /* Clear multicast filter */
    wr32(dev->mmio, IXGBE_REG_MCSTCTRL, 0);

    return 0;
}

/*
 * Enable Rx
 */
static __inline__ int
ixgbe_enable_rx(struct ixgbe_device *dev)
{
    /* Enable RX */
    wr32(dev->mmio, IXGBE_REG_RXCTL,
         rd32(dev->mmio, IXGBE_REG_RXCTL) | IXGBE_RXCTL_RXEN);

    return 0;
}

/*
 * Disable Rx
 */
static __inline__ int
ixgbe_disable_rx(struct ixgbe_device *dev)
{
    /* Disable RX */
    wr32(dev->mmio, IXGBE_REG_RXCTL,
         rd32(dev->mmio, IXGBE_REG_RXCTL) & ~IXGBE_RXCTL_RXEN);

    return 0;
}

/*
 * Setup Rx ring
 */
static __inline__ int
ixgbe_setup_rx_ring(struct ixgbe_device *dev, struct ixgbe_rx_ring *rxring,
                    void *m, uint64_t v2poff, uint16_t qlen)
{
    union ixgbe_rx_desc *rxdesc;
    ssize_t i;
    uint32_t m32;
    uint64_t m64;
    struct timespec tm;

    rxring->mmio = dev->mmio;

    rxring->tail = 0;
    rxring->head = 0;

    /* up to 64 K minus 8 */
    rxring->len = qlen;

    /* Allocate for descriptors */
    rxring->descs = m;
    m += sizeof(union ixgbe_rx_desc) * qlen;
    rxring->bufs = m;

    for ( i = 0; i < rxring->len; i++ ) {
        rxdesc = &rxring->descs[i];
        rxdesc->read.pkt_addr = 0;
        rxdesc->read.hdr_addr = 0;
    }

    m64 = (uint64_t)rxring->descs + v2poff;
    wr32(rxring->mmio, IXGBE_REG_RDBAL(rxring->idx), m64 & 0xffffffffULL);
    wr32(rxring->mmio, IXGBE_REG_RDBAH(rxring->idx), m64 >> 32);
    wr32(rxring->mmio, IXGBE_REG_RDLEN(rxring->idx),
         rxring->len * sizeof(union ixgbe_rx_desc));

    wr32(rxring->mmio, IXGBE_REG_SRRCTL(rxring->idx),
         IXGBE_SRRCTL_BSIZE_PKT10K | (1 << 25) | (1 << 28) | (0 << 22));

    /* Enable this queue */
    wr32(rxring->mmio, IXGBE_REG_RXDCTL(rxring->idx),
         IXGBE_RXDCTL_ENABLE/* | IXGBE_RXDCTL_VME*/);
    for ( i = 0; i < 10; i++ ) {
        tm.tv_sec = 0;
        tm.tv_nsec = 1000;
        nanosleep(&tm, NULL);
        m32 = rd32(rxring->mmio, IXGBE_REG_RXDCTL(rxring->idx));
        if ( m32 & IXGBE_RXDCTL_ENABLE ) {
            break;
        }
    }
    if ( !(m32 & IXGBE_RXDCTL_ENABLE) ) {
        printf("Error on enable an RX queue.\n");
    }

    wr32(rxring->mmio, IXGBE_REG_RDH(rxring->idx), 0);
    wr32(rxring->mmio, IXGBE_REG_RDT(rxring->idx), 0);

    return 0;
}

/*
 * Setup Tx port
 */
static __inline__ int
ixgbe_setup_tx(struct ixgbe_device *dev)
{
    /* Do nothing */
    (void)dev;

    return 0;
}

/*
 * Enable Tx
 */
static __inline__ int
ixgbe_enable_tx(struct ixgbe_device *dev)
{
    /* Enable Tx */
    wr32(dev->mmio, IXGBE_REG_DMATXCTL,
         IXGBE_DMATXCTL_TE | IXGBE_DMATXCTL_VT);

    return 0;
}

/*
 * Disable Tx
 */
static __inline__ int
ixgbe_disable_tx(struct ixgbe_device *dev)
{
    /* Disable Tx */
    wr32(dev->mmio, IXGBE_REG_DMATXCTL,
         rd32(dev->mmio, IXGBE_REG_DMATXCTL) & ~IXGBE_DMATXCTL_TE);

    return 0;
}

/*
 * Setup Tx ring
 */
static __inline__ int
ixgbe_setup_tx_ring(struct ixgbe_device *dev, struct ixgbe_tx_ring *txring,
                    void *m, uint64_t v2poff, uint16_t qlen)
{
    union ixgbe_tx_desc *txdesc;
    ssize_t i;
    uint32_t m32;
    uint64_t m64;
    struct timespec tm;

    txring->mmio = dev->mmio;

    txring->tail = 0;
    txring->head = 0;
    txring->len = qlen;

    /* Allocate for descriptors */
    txring->descs = m;
    m += sizeof(union ixgbe_tx_desc) * qlen;
    txring->bufs = m;
    m += sizeof(void *) * qlen;
    txring->tdwba = m;

    for ( i = 0; i < txring->len; i++ ) {
        txdesc = &txring->descs[i];
        txdesc->data.pkt_addr = 0;
        txdesc->data.length = 0;
        txdesc->data.dtyp_mac = 0;
        txdesc->data.dcmd = 0;
        txdesc->data.paylen_popts_cc_idx_sta = 0;
    }

    m64 = (uint64_t)txring->descs + v2poff;
    wr32(txring->mmio, IXGBE_REG_TDBAH(txring->idx), m64 >> 32);
    wr32(txring->mmio, IXGBE_REG_TDBAL(txring->idx), m64 & 0xffffffffUL);
    wr32(txring->mmio, IXGBE_REG_TDLEN(txring->idx),
         txring->len * sizeof(union ixgbe_tx_desc));
    wr32(txring->mmio, IXGBE_REG_TDH(txring->idx), 0);
    wr32(txring->mmio, IXGBE_REG_TDT(txring->idx), 0);

    /* Write-back */
    m64 = (uint64_t)txring->tdwba + v2poff;
    wr32(txring->mmio, IXGBE_REG_TDWBAH(txring->idx), m64 >> 32);
    wr32(txring->mmio, IXGBE_REG_TDWBAL(txring->idx), (m64 & 0xfffffffc) | 1);
    *(txring->tdwba) = 0;

    /* P+W <= 40  */
    wr32(txring->mmio, IXGBE_REG_TXDCTL(txring->idx), IXGBE_TXDCTL_ENABLE
         | (16 << 16) /* WTHRESH */
         | (8 << 8) /* HTHRESH */
         | (16) /* PTHRESH */);
    for ( i = 0; i < 10; i++ ) {
        tm.tv_sec = 0;
        tm.tv_nsec = 1000;
        nanosleep(&tm, NULL);
        m32 = rd32(txring->mmio, IXGBE_REG_TXDCTL(txring->idx));
        if ( m32 & IXGBE_TXDCTL_ENABLE ) {
            break;
        }
    }
    if ( !(m32 & IXGBE_TXDCTL_ENABLE) ) {
        printf("Error on enable a TX queue.\n");
    }

    return 0;
}


static __inline__ int
ixgbe_calc_rx_ring_memsize(struct ixgbe_rx_ring *rx, uint16_t qlen)
{
    (void)rx;
    return (sizeof(union ixgbe_rx_desc) + sizeof(void *)) * qlen;
}
static __inline__ int
ixgbe_calc_tx_ring_memsize(struct ixgbe_tx_ring *tx, uint16_t qlen)
{
    (void)tx;
    return (sizeof(union ixgbe_tx_desc) + sizeof(void *)) * qlen + 128;
}

#endif /* _IXGBE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
