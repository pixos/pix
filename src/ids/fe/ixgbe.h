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

#include <stdint.h>
#include <stdlib.h>
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
#define IXGBE_REG_MTA           0x5200  /* x128 */
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
#define IXGBE_CTRL_PCIE_MASTER_DISABLE  (u32)(1<<2)
#define IXGBE_CTRL_RST  (1<<26)

#define IXGBE_STATUS_PCIE_MASTER_ENABLE  (u32)(1<<19)

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


struct ixgbe_rx_ring {
    union ixgbe_rx_desc *descs;
    uint16_t tail;
    uint16_t head;
};
struct ixgbe_tx_ring {
    union ixgbe_tx_desc *descs;
};

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

    /* Get the device MAC address */
    ixgbe_read_mac_address(dev);

    return dev;
}

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

#endif /* _IXGBE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
