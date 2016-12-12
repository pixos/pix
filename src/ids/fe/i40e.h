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

#ifndef _I40E_H
#define _I40E_H

#include <stdint.h>
#include <mki/driver.h>
#include "pci.h"
#include "common.h"

#define I40E_XL710QDA1          0x1584
#define I40E_XL710QDA2          0x1583

#define I40E_MMIO_SIZE          0x200000
#define I40E_PFLAN_QALLOC       0x001c0400  /* RO */

#define I40E_GLLAN_TXPRE_QDIS(n) (0x000e6500 + 0x4 * (n))

#define I40E_QTX_ENA(q)         (0x00100000 + 0x4 * (q))
#define I40E_QTX_CTL(q)         (0x00104000 + 0x4 * (q))
#define I40E_QTX_HEAD(q)        (0x000e4000 + 0x4 * (q))
#define I40E_QTX_TAIL(q)        (0x00108000 + 0x4 * (q))

#define I40E_QRX_ENA(q)         (0x00120000 + 0x4 * (q))
#define I40E_QRX_TAIL(q)        (0x00128000 + 0x4 * (q))

#define I40E_GLHMC_LANTXBASE(n) (0x000c6200 + 0x4 * (n)) /* [0:23] */
#define I40E_GLHMC_LANTXCNT(n)  (0x000c6300 + 0x4 * (n)) /* [0:10] */
#define I40E_GLHMC_LANRXBASE(n) (0x000c6400 + 0x4 * (n)) /* [0:23] */
#define I40E_GLHMC_LANRXCNT(n)  (0x000c6500 + 0x4 * (n)) /* [0:10] */

#define I40E_GLHMC_LANTXOBJSZ   0x000c2004 /* [0:3] RO */
#define I40E_GLHMC_LANRXOBJSZ   0x000c200c /* [0:3] RO */

#define I40E_PFHMC_SDCMD        0x000c0000
#define I40E_PFHMC_SDDATALOW    0x000c0100
#define I40E_PFHMC_SDDATAHIGH   0x000c0200
#define I40E_PFHMC_PDINV        0x000c0300
#define I40E_GLHMC_SDPART(n)    (0x000c0800 + 0x04 * (n)) /* RO */


#define I40E_PRTPM_SAL(n)       (0x001e4440 + 0x20 * (n)) /* RO */
#define I40E_PRTPM_SAH(n)       (0x001e44c0 + 0x20 * (n)) /* RO */

#define I40E_GLPCI_CNF2         0x000be494
#define I40E_GLPCI_LINKCAP      0x000be4ac
#define I40E_GLSCD_QUANTA       0x000b2080

#define I40E_GLLAN_RCTL_0       0x0012a500

#define I40E_GLPRT_GOTC(n)      (0x00300680 + 0x8 * (n))

#define I40E_PRTGL_SAL          0x001e2120
#define I40E_PRTGL_SAH          0x001e2140

#define I40E_GLGEN_RTRIG        0x000b8190

#define I40E_AQ_LARGE_BUF       512 /* MAX ==> 4096 */

#define I40E_PF_ATQBAL          0x00080000
#define I40E_PF_ATQBAH          0x00080100
#define I40E_PF_ATQLEN          0x00080200      /*enable:1<<31*/
#define I40E_PF_ATQH            0x00080300
#define I40E_PF_ATQT            0x00080400

#define I40E_PF_ARQBAL          0x00080080
#define I40E_PF_ARQBAH          0x00080180
#define I40E_PF_ARQLEN          0x00080280      /*enable:1<<31*/
#define I40E_PF_ARQH            0x00080380
#define I40E_PF_ARQT            0x00080480

#define I40E_AQ_LEN             128
#define I40E_AQ_BUF             4096

#define I40E_HMC_SIZE           (4 * 1024 * 1024)

/*
 * Receive descriptor
 */
struct i40e_rx_desc_read {
    uint64_t pkt_addr;
    volatile uint64_t hdr_addr; /* Bit 0: DD */
} __attribute__ ((packed));
struct i40e_rx_desc_wb {
    uint32_t filter_stat;
    uint32_t l2tag_mirr_fcoe_ctx;
    uint64_t len_ptype_err_status;
} __attribute__ ((packed));
union i40e_rx_desc {
    struct i40e_rx_desc_read read;
    struct i40e_rx_desc_wb wb;
} __attribute__ ((packed));

/*
 * Transmit descriptor
 */
struct i40e_tx_desc_ctx {
    uint32_t tunnel;            /* rsv:8, Tunneling parameters:24 */
    uint16_t l2tag;             /* L2TAG2 (STag/VEXT) */
    uint16_t rsv;
    uint64_t mss_tsolen_cmd_dtyp;
} __attribute__ ((packed));
struct i40e_tx_desc_data {
    uint64_t pkt_addr;
    uint16_t rsv_cmd_dtyp;      /* rsv:2, cmd:10, dtyp:4 */
    uint32_t txbufsz_offset;    /* Tx buffer size:14, offset:18 */
    uint16_t l2tag;
} __attribute__ ((packed));
union i40e_tx_desc {
    struct i40e_tx_desc_ctx ctx;
    struct i40e_tx_desc_data data;
} __attribute__ ((packed));

/*
 * Admin queue
 */
struct i40e_aq_desc {
    volatile uint16_t flags;
    uint16_t opcode;
    uint16_t len;
    uint16_t ret;
    uint32_t cookieh;
    uint32_t cookiel;
    uint32_t param0;
    uint32_t param1;
    uint32_t addrh;
    uint32_t addrl;
} __attribute__ ((packed));

/*
 * Admin Tx queue
 */
struct i40e_atq {
    struct i40e_aq_desc *descs;
    void *base;
    uint16_t tail;
    uint16_t len;
    uint8_t *bufset;
    void *pbufset;
};

/*
 * Admin Rx queue
 */
struct i40e_arq {
    struct i40e_aq_desc *descs;
    void *base;
    uint16_t tail;
    uint16_t len;
    uint8_t *bufset;
    void *pbufset;
};

/*
 * i40e device
 */
struct i40e_device {
    void *mmio;
    uint8_t macaddr[6];
    uint16_t device_id;

    /* Version */
    struct {
        int major;
        int minor;
        int build;
        int subbuild;
    };

    /* Admin queue */
    struct i40e_atq atq;
    struct i40e_arq arq;

    /* HMC */
    void *hmc;
};




/*
 * LAN Rx queue context
 */
struct i40e_lan_rxq_ctx {
    /* 0-31 */
    uint32_t head:13;
    uint32_t cpuid:8;
    uint32_t rsv1:11;
    /* 32-95 */
    uint64_t base:57;
    uint64_t qlen:13;
    /* 96-127 */
    uint32_t dbuff:7;
    uint32_t hbuff:5;
    uint32_t dtype:2;
    uint32_t dsize:1;
    uint32_t crcstrip:1;
    uint32_t fcena:1;
    uint32_t l2tsel:1;
    uint32_t hsplit_0:4;
    uint32_t hsplit_1:2;
    uint32_t rsv2:1;
    uint32_t showiv:1;
    /* 128-191 */
    uint64_t rsv3:46;
    uint64_t rxmax:14;
    uint64_t rsv4l:4;
    /* 192- */
    uint64_t rsv4h:1;
    uint64_t tphrdesc:1;
    uint64_t tphwdesc:1;
    uint64_t tphdata:1;
    uint64_t tphhead:1;
    uint64_t rsv5:1;
    uint64_t lrxqtresh:3;
    uint64_t rsv6:55;
} __attribute__ ((packed));

/*
 * LAN Tx queue context
 */
struct i40e_lan_txq_ctx {
    /* Line 0.0 */
    uint32_t head:13;
    uint32_t rsv1:17;
    uint32_t newctx:1;
    uint32_t rsv2:1;
    /* Line 0.1 */
    uint64_t base:57;
    uint64_t fcena:1;
    uint64_t tsynena:1;
    uint64_t fdena:1;
    uint64_t alt_vlan:1;
    uint64_t rsv3:3;
    /* Line 0.2 */
    uint32_t cpuid:8;
    uint32_t rsv4:24;
    /* Line 1.0 */
    uint32_t thead_wb:13;
    uint32_t rsv5:19;
    /* Line 1.1 */
    uint32_t head_wben:1;
    uint32_t qlen:13;
    uint32_t tphrdesc:1;
    uint32_t tphrpacket:1;
    uint32_t tphwdesc:1;
    uint32_t rsv6:15;
    /* Line 1.2 */
    uint64_t head_wbaddr;
    /* Line 2 */
    uint64_t rsv7[2];
    /* Line 3 */
    uint64_t rsv8[2];
    /* Line 4 */
    uint64_t rsv9[2];
    /* Line 5 */
    uint64_t rsv10[2];
    /* Line 6 */
    uint64_t rsv11[2];
    /* Line 7.0 */
    uint64_t rsv12;
    /* Line 7.1 */
    uint64_t rsv13:20;
    uint64_t rdylist:10;
    uint64_t rsv14:34;
} __attribute__ ((packed));


struct i40e_adm_get_switch_config_hdr {
    uint16_t num_elem;
    uint16_t total_num;
    uint8_t rsv[12];
} __attribute__ ((packed));
struct i40e_adm_get_switch_config_elem {
    uint8_t elem_type;          /* VSI = 0x13 */
    uint8_t revision;           /* always 1 in XL710 */
    uint16_t seid;
    uint16_t uplink;
    uint16_t downlink;
    uint8_t rsv1[3];
    uint8_t conn_type;
    uint8_t rsv2[2];
    uint16_t elem_spec;
} __attribute__ ((packed));

/* Prototype declarations */
int i40e_read_mac_address(struct i40e_device *);

#endif /* _I40E_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
