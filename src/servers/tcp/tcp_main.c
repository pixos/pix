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
#include <limits.h>
#include <fcntl.h>
#include <time.h>

/* TCP options */
#define TCP_OPT_EOL             0
#define TCP_OPT_NOP             1
#define TCP_OPT_MSS             2
#define TCP_OPT_WSCALE          3
#define TCP_OPT_SACK_PERMITTED  4
#define TCP_OPT_SACK            5
#define TCP_OPT_TIMESTAMP       8
#define TCP_OPT_ALT_CS_REQ      14
#define TCP_OPT_ALT_CS_DATA     15

/* TCP state */
enum tcp_state {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_RECEIVED,
    TCP_SYN_SENT,
    /* Established */
    TCP_ESTABLISHED,
    /* Active close */
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_CLOSING,
    TCP_TIME_WAIT,
    /* Passive close */
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
};

/* TCP options */
struct tcpopt {
    int mss;
    int wscale;
};

/* TCP session */
struct tcp_session {
    enum tcp_state state;
    /* Receive window */
    struct {
        uint32_t sz;
        uint8_t *buf;
        /* Sliding window */
        uint32_t pos0;
        uint32_t pos1;
    } rwin;
    /* Transmit window */
    struct {
        uint32_t sz;
        uint8_t *buf;
        uint32_t pos0;
        uint32_t pos1;
    } twin;
    /* Local/Remote IP address/port number */
    uint32_t lipaddr;
    uint16_t lport;
    uint32_t ripaddr;
    uint16_t rport;
    /* seq/ack numbers */
    uint32_t seq;
    uint32_t ack;
    uint32_t rseqno;
    uint32_t rackno;
    /* Received window size */
    uint16_t rcvwin;
    /* MSS */
    uint32_t mss;
    /* Window */
    uint8_t wscale;
};

/* TCP header */
struct tcp_hdr {
    uint16_t sport;
    uint16_t dport;
    uint32_t seqno;
    uint32_t ackno;
    uint8_t flag_ns:1;
    uint8_t flag_reserved:3;
    uint8_t offset:4;
    uint8_t flag_fin:1;
    uint8_t flag_syn:1;
    uint8_t flag_rst:1;
    uint8_t flag_psh:1;
    uint8_t flag_ack:1;
    uint8_t flag_urg:1;
    uint8_t flag_ece:1;
    uint8_t flag_cwr:1;
    uint16_t wsize;
    uint16_t checksum;
    uint16_t urgptr;
} __attribute__ ((packed));

/* IPv4 TCP pseudo header */
struct tcp_phdr4 {
    uint32_t saddr;
    uint32_t daddr;
    uint8_t zeros;
    uint8_t proto;
    uint16_t tcplen;
    uint16_t sport;
    uint16_t dport;
    uint32_t seqno;
    uint32_t ackno;
    uint8_t rsvd:4;
    uint8_t offset:4;
    uint8_t flags;
    uint16_t wsize;
    uint16_t checksum;
    uint16_t urgptr;
} __attribute__ ((packed));



/*
 * Compute checksum
 */
static __inline__ uint16_t
_checksum(const uint8_t *buf, int len)
{
    int nleft;
    uint32_t sum;
    const uint16_t *cur;
    union {
        uint16_t us;            /* unsigned short */
        uint8_t uc[2];          /* unsigned char x2 */
    } last;
    uint16_t ret;

    nleft = len;
    sum = 0;
    cur = (const uint16_t *)buf;
    while ( nleft > 1 ) {
        sum += (uint32_t)*cur;
        sum = (sum & 0xffff) + (sum >> 16);
        nleft -= 2;
        cur += 1;
    }
    if ( 1 == nleft ) {
        last.uc[0] = *(const uint8_t *)cur;
        last.uc[1] = 0;
        sum += last.us;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    ret = ~sum;

    return ret;
}

/*
 * Compute checksume (2-byte aligned size)
 */
static __inline__ uint16_t
_ip_checksum(const uint8_t *data, int len)
{
    const uint16_t *tmp;
    uint32_t cs;
    int i;

    tmp = (const uint16_t *)data;
    cs = 0;
    for ( i = 0; i < len / 2; i++ ) {
        cs += (uint32_t)tmp[i];
        cs = (cs & 0xffff) + (cs >> 16);
    }
    cs = 0xffff - cs;

    return cs;
}

/*
 * Send
 */
int
tcp_send(struct tcp_session *sess, const uint8_t *pkt, size_t len)
{
    size_t bufsz;

    /* Check the buffer to push the new data */
    bufsz = (sess->twin.sz + sess->twin.pos0 - sess->twin.pos1 - 1)
        % sess->twin.sz;

    if ( bufsz < len ) {
        /* No buffer available */
        return -1;
    }

    /* Enqueue the packet data to the buffer */
    if ( sess->twin.pos1 + len > sess->twin.sz ) {
        memcpy(sess->twin.buf + sess->twin.pos1, pkt,
               sess->twin.sz - sess->twin.pos1);
        memcpy(sess->twin.buf, pkt + sess->twin.sz - sess->twin.pos1,
               len - sess->twin.sz + sess->twin.pos1);
    } else {
        memcpy(sess->twin.buf + sess->twin.pos1, pkt, len);
    }

    sess->twin.pos1 = (sess->twin.pos1 + len) % sess->twin.sz;

    return 0;
}

/*
 * Trigger
 */
int
tcp_trigger(struct tcp_session *sess)
{
    return -1;
}

/*
 * Entry point for the tty driver
 */
int
main(int argc, char *argv[])
{
    struct timespec tm;

    /* Loop */
    tm.tv_sec = 1;
    tm.tv_nsec = 0;
    for ( ;; ) {
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
