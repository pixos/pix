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
    uint64_t mmio;
    struct ixgbe_rx_ring *rxq[IXGBE_NRXQ];
    struct ixgbe_tx_ring *txq[IXGBE_NTXQ];
    uint8_t macaddr[6];
};



#endif /* _IXGBE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
