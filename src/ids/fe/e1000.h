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

#ifndef _E1000_H
#define _E1000_H

#include <stdint.h>

#define E1000_NRXQ              1
#define E1000_NTXQ              1

/* MMIO registers */
#define E1000_REG_CTRL          0x00
#define E1000_REG_STATUS        0x08
#define E1000_REG_EEC           0x10
#define E1000_REG_CTRL_EXT      0x18
#define E1000_REG_EERD          0x14
#define E1000_REG_MDIC          0x20
#define E1000_REG_ICR           0x00c0
#define E1000_REG_ICS           0x00c8
#define E1000_REG_IMS           0x00d0
#define E1000_REG_IMC           0x00d8
#define E1000_REG_RCTL          0x0100
#define E1000_REG_RDBAL         0x2800
#define E1000_REG_RDBAH         0x2804
#define E1000_REG_RDLEN         0x2808
#define E1000_REG_RDH           0x2810  /* head */
#define E1000_REG_RDT           0x2818  /* tail */
#define E1000_REG_TCTL          0x0400  /* transmit control */
#define E1000_REG_TDBAL         0x3800
#define E1000_REG_TDBAH         0x3804
#define E1000_REG_TDLEN         0x3808
#define E1000_REG_TDH           0x3810  /* head */
#define E1000_REG_TDT           0x3818  /* tail */
#define E1000_REG_MTA           0x5200  /* x128 */
#define E1000_REG_TXDCTL        0x03828
#define E1000_REG_RAL           0x5400
#define E1000_REG_RAH           0x5404

#define E1000_CTRL_FD           1       /* Full duplex */
#define E1000_CTRL_LRST         (1<<3)  /* Link reset */
#define E1000_CTRL_ASDE         (1<<5)  /* Auto speed detection enable */
#define E1000_CTRL_SLU          (1<<6)  /* Set linkup */
#define E1000_CTRL_PHY_RST      (1<<31)  /* PHY reset */

#define E1000_CTRL_RST          (1<<26)
#define E1000_CTRL_VME          (1<<30)

#define E1000_CTRL_EXT_LINK_MODE_MASK (3<<22)
#define E1000_CTRL_EXT_EE_RST   (1<<13)

#define E1000_RCTL_EN           (1<<1)
#define E1000_RCTL_SBP          (1<<2)
#define E1000_RCTL_UPE          (1<<3)  /* Unicast promiscuous */
#define E1000_RCTL_MPE          (1<<4)  /* Multicast promiscuous */
#define E1000_RCTL_LPE          (1<<5)  /* Long packet reception */
#define E1000_RCTL_BAM          (1<<15) /* Broadcast accept mode */
#define E1000_RCTL_BSEX         (1<<25) /* Buffer size extension */
#define E1000_RCTL_SECRC        (1<<26) /* Strip ethernet CRC from incoming packet */

#define E1000_RCTL_BSIZE_8192   ((2<<16) | E1000_RCTL_BSEX)
#define E1000_RCTL_BSIZE_SHIFT  16

#define E1000_TCTL_EN           (1<<1)
#define E1000_TCTL_PSP          (1<<3)  /* pad short packets */
#define E1000_TCTL_MULR         (1<<28)

#define E1000_TXDCTL_GRAN_CL    0
#define E1000_TXDCTL_GRAN_DESC  (1<<24)
#define E1000_TXDCTL_PTHRESH_SHIFT 0
#define E1000_TXDCTL_HTHRESH_SHIFT 8
#define E1000_TXDCTL_WTHRESH_SHIFT 16
#define E1000_TXDCTL_LTHRESH_SHIFT 25

#define E1000_PRO1000MT         0x100e  /* Intel Pro 1000/MT */
#define E1000_82545EM           0x100f
#define E1000_82541PI           0x107c
#define E1000_82573L            0x109a
#define E1000_82567LM           0x10f5
#define E1000_82577LM           0x10ea
#define E1000_82579LM           0x1502




struct e1000_rx_desc {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__ ((packed));
struct e1000_tx_desc {
    volatile uint64_t address;
    uint32_t length:20;
    uint32_t dtyp:4;
    uint32_t dcmd:8;
    volatile uint8_t sta:4;
    uint8_t rsv:4;
    uint8_t popts;
    uint16_t special;
} __attribute__ ((packed));

struct e1000_rx_ring {
    struct e1000_rx_desc *descs;
    void *bufs;
    uint16_t tail;
    uint16_t head;
    uint16_t len;
};
struct e1000_tx_ring {
    struct e1000_tx_desc *descs;
    void *bufs;
    uint16_t tail;
    uint16_t len;
};
struct e1000_device {
    uint64_t mmio;
    struct e1000_rx_ring *rxq[E1000_NRXQ];
    struct e1000_tx_ring *txq[E1000_NTXQ];
    uint8_t macaddr[6];
    struct pci_dev *pci_device;
};

#endif /* _E1000_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
