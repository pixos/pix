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

#ifndef _SYS_NET_ETHERNET_H
#define _SYS_NET_ETHERNET_H

#define ETHER_ADDR_LEN          6
#define ETHER_TYPE_LEN          2
#define ETHER_CRC_LEN           4
#define ETHER_HDR_LEN           (ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN)
#define ETHER_MIN_LEN           64
#define ETHER_MAX_LEN           1518
#define ETHER_MAX_LEN_JUMBO     9018

#define ETHER_VLAN_ENCAP_LEN    4

#include <stdint.h>

/*
 * Ethernet header
 */
struct ether_header {
    uint8_t     ether_dhost[ETHER_ADDR_LEN];
    uint8_t     ether_shost[ETHER_ADDR_LEN];
    uint16_t    ether_type;
} __attribute__ ((packed));

#define ETHER_IS_MULTICAST(addr) (*(addr) & 0x01) /* Is address mcast/bcast? */

/*
 * 802.1Q VLAN header.
 */
struct ether_vlan_header {
    uint8_t     evl_dhost[ETHER_ADDR_LEN];
    uint8_t     evl_shost[ETHER_ADDR_LEN];
    uint16_t    evl_encap_proto;
    uint16_t    evl_tag;
    uint16_t    evl_proto;
} __attribute__ ((packed));


#endif /* _SYS_NET_ETHERNET_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
