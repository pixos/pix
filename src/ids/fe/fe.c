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
#include <sys/syscall.h>
#include <sys/mman.h>
#include "pci.h"


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

void
sysxpsleep(void)
{
    __asm__ __volatile__ ("syscall" :: "a"(SYS_xpsleep));
}

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    char buf[512];
    int ret;
    char *mem;

    /* Map */
    mem = malloc(4096);;
    strcpy(mem, "abcd efgh");

    while ( 1 ) {
        ret = pci_read_config(0, 0, 0, 0);
        snprintf(buf, 512, "xxx %s %x %p %s\r\n", "abcd", ret, mem, mem);
        write(1, buf, strlen(buf));
        sysxpsleep();
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
