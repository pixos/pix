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

#include <stdlib.h>
#include <machine/sysarch.h>
#include <mki/driver.h>
#include "tty.h"

/*
 * Initialize a serial console port
 */
int
serial_init(struct serial *serial, int nr, const char *ttyname)
{
    struct driver_mapped_device *dev;
    struct sysarch_io io;

    /* Determine the port from the serial port number */
    switch ( nr ) {
    case 0:
        serial->port = 0x3f8;
        serial->irq = 4;
        break;
    case 1:
        serial->port = 0x2f8;
        serial->irq = 3;
        break;
    case 2:
        serial->port = 0x3e8;
        serial->irq = 4;
        break;
    case 3:
        serial->port = 0x2e8;
        serial->irq = 3;
        break;
    default:
        return -1;
    }


    /* Initialize the serial port */

    /* Disable all interrupts */
    io.port = serial->port + 1;
    io.data = 0x00;
    sysarch(SYSARCH_OUTB, &io);

    /* Enable Divisor Latch Access Bit (DLAB) and set baud rate divisor to 12;
       i.e., set baud rate to 115200 / 12 = 9600 */
    io.port = serial->port + 3;
    io.data = 0x80;
    sysarch(SYSARCH_OUTB, &io);

    io.port = serial->port + 0; /* Low byte */
    io.data = 0x0c;
    sysarch(SYSARCH_OUTB, &io);

    io.port = serial->port + 1; /* High byte */
    io.data = 0x00;
    sysarch(SYSARCH_OUTB, &io);

    /* Set the mode to 8 bits, no parity, one stop bit */
    io.port = serial->port + 3;
    io.data = 0x03;
    sysarch(SYSARCH_OUTB, &io);

    /* Enable FIFO */
    io.port = serial->port + 2;
    io.data = 0xc7;
    sysarch(SYSARCH_OUTB, &io);

    /* Enable IRQs, and set RTS/DSR */
    io.port = serial->port + 4;
    io.data = 0x0b;
    sysarch(SYSARCH_OUTB, &io);

    /* Register an interrupt handler */
    driver_register_irq_handler(serial->irq, NULL);

    /* Register serial device as a character device */
    dev = driver_register_device(ttyname, 0);
    serial->dev = dev;

    return 0;
}

int
serial_proc(struct serial *serial)
{
    struct sysarch_io io;
    int ascii;
    off_t next_tail;

    for ( ;; ) {
        /* Received? */
        io.port = serial->port + 5;
        sysarch(SYSARCH_INB, &io);
        if ( !(io.data & 1) ) {
            break;
        }

        /* Read */
        io.port = serial->port;
        sysarch(SYSARCH_INB, &io);
        ascii = io.data;

        next_tail = serial->dev->dev.chr.ibuf.tail + 1;
        next_tail = next_tail < 512 ? next_tail : 0;
        if ( serial->dev->dev.chr.ibuf.head == next_tail ) {
            /* Buffer full */
            break;
        }
        /* Enqueue to the buffer */
        serial->dev->dev.chr.ibuf.buf[serial->dev->dev.chr.ibuf.tail] = ascii;
        __asm__ __volatile__ ("mfence");
        serial->dev->dev.chr.ibuf.tail = next_tail;
        __asm__ __volatile__ ("mfence");

        if ( '\r' == ascii ) {
            driver_interrupt(serial->dev);
        }
    }

    /* Read line state to until the transmit buffer is empty */
    for ( ;; ) {
        io.port = serial->port + 5;
        sysarch(SYSARCH_INB, &io);
        if ( 0 != (io.data & 0x20) ) {
            break;
        }
    }
    while ( serial->dev->dev.chr.obuf.head
            != serial->dev->dev.chr.obuf.tail ) {

        /* and echo back */
        io.port = serial->port;
        io.data
            = serial->dev->dev.chr.obuf.buf[serial->dev->dev.chr.obuf.head];
        sysarch(SYSARCH_OUTB, &io);
        serial->dev->dev.chr.obuf.head++;
        serial->dev->dev.chr.obuf.head
            = serial->dev->dev.chr.obuf.head < 512
            ? serial->dev->dev.chr.obuf.head : 0;
    }

    return 0;
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
