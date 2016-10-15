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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#include <machine/sysarch.h>
#include <mki/driver.h>
#include "kbd.h"

#define KBD_OK          0
#define KBD_ERROR       -1
#define KBD_MAX_RETRY   0x01000000

#define KBD_ENCODER     0x0060
#define KBD_ENC_BUFFER  KBD_ENCODER
#define KBD_ENC_COMMAND KBD_ENCODER

#define KBD_CONTROLLER  0x0064
#define KBD_CTRL_STATUS KBD_CONTROLLER
#define KBD_CTRL_COMMAND KBD_CONTROLLER

/* Keyboard encoder */
#define KBD_ENC            0x0060
#define KBD_ENC_BUF        KBD_ENC
#define KBD_ENC_CMD        KBD_ENC

/* Controller */
#define KBD_CTRL           0x0064
#define KBD_CTRL_STAT      KBD_CTRL
#define KBD_CTRL_CMD       KBD_CTRL

#define KBD_STAT_OBUF 0x01
#define KBD_STAT_IBUF 0x02

#define KBD_ENC_CMD_SETLED      0xed
#define KBD_ENC_CMD_ENABLE      0xF4
#define KBD_ENC_CMD_DISABLE     0xF5

/* LED status */
#define KBD_LED_NONE            0x00000000
#define KBD_LED_SCROLLLOCK      0x00000001
#define KBD_LED_NUMLOCK         0x00000002
#define KBD_LED_CAPSLOCK        0x00000004


/* Commands */
#define KBD_CTRL_CMD_DISABLE    0xad
#define KBD_CTRL_CMD_ENABLE     0xae
#define KBD_CTRL_CMD_SELFTEST   0xaa

/* Status of self test */
#define KBD_CTRL_STAT_SELFTEST_OK   0x55
#define KBD_CTRL_STAT_SELFTEST_NG   0xfc

/*
 * Prototype declarations
 */
int kbd_wait_until_outbuf_full(void);


/*
 * Read control status
 */
unsigned char
kbd_read_ctrl_status(void)
{
    struct sysarch_io io;

    io.port = KBD_CTRL_STAT;
    sysarch(SYSARCH_INB, &io);
    return io.data;
}

/*
 * Write a control command
 */
int
kbd_write_ctrl_cmd(unsigned char cmd)
{
    int retry;
    struct sysarch_io io;

    /* Retry until it succeeds or exceeds retry max */
    for ( retry = 0; retry < KBD_MAX_RETRY; retry++ ) {
        if ( 0 == (kbd_read_ctrl_status() & KBD_STAT_IBUF) ) {
            io.port = KBD_CTRL_CMD;
            io.data = cmd;
            sysarch(SYSARCH_OUTB, &io);
            return KBD_OK;
        }
    }

    return KBD_ERROR;
}

/*
 * Read from keyboard encoder
 */
unsigned char
kbd_enc_read_buf(void)
{
    struct sysarch_io io;

    io.port = KBD_ENC_BUF;
    sysarch(SYSARCH_INB, &io);
    return io.data;
}

/*
 * Write command to the keyboard encoder
 */
int
kbd_enc_write_cmd(unsigned char cmd)
{
    int retry;
    struct sysarch_io io;

    for ( retry = 0; retry < KBD_MAX_RETRY; retry++ ) {
        if ( 0 == (kbd_read_ctrl_status() & KBD_STAT_IBUF) ) {
            io.port = KBD_ENC_CMD;
            io.data = cmd;
            sysarch(SYSARCH_OUTB, &io);
            return KBD_OK;
        }
    }

    return KBD_ERROR;
}

/*
 * Set LED status
 */
int
kbd_set_led(int led)
{
    int stat;

    /* Check the argument value */
    if ( ( KBD_LED_SCROLLLOCK | KBD_LED_NUMLOCK | KBD_LED_CAPSLOCK ) < led ) {
        return KBD_ERROR;
    }

    stat = kbd_enc_write_cmd(KBD_ENC_CMD_SETLED);
    stat |= kbd_enc_write_cmd((unsigned char)led);

    return stat;
}

/*
 * Run self test
 */
int
kbd_selftest(void)
{
    unsigned char encbuf;
    int stat;

    stat = kbd_write_ctrl_cmd(KBD_CTRL_CMD_SELFTEST);
    if ( KBD_OK != stat ) {
        return stat;
    }

    /* Wait until output buffer becomes full */
    stat = kbd_wait_until_outbuf_full();
    if ( KBD_OK != stat ) {
        return stat;
    }

    /* Check the self-test result */
    encbuf = kbd_enc_read_buf();
    if ( KBD_CTRL_STAT_SELFTEST_OK == encbuf ) {
        /* KBD_OK */
        return stat;
    }

    return KBD_ERROR;
}

/*
 * Wait until the output buffer becomes full
 */
int
kbd_wait_until_outbuf_full(void)
{
    unsigned char stat;
    int retry;

    for ( retry = 0; retry < KBD_MAX_RETRY; retry++ ) {
        stat = kbd_read_ctrl_status();

        if ( KBD_STAT_OBUF == (stat & KBD_STAT_OBUF) ) {
            return KBD_OK;
        }
    }

    return KBD_ERROR;
}

/*
 * Disable keyboard
 */
int
kbd_disable(struct kbd *kbd)
{
    int stat;

    /* Disable keyboard */
    stat = kbd_write_ctrl_cmd(KBD_CTRL_CMD_DISABLE);

    if ( KBD_OK != stat ) {
        kbd->disabled = 0;
    } else {
        kbd->disabled = 1;
    }

    return stat;
}

/*
 * Enable keyboard
 */
int
kbd_enable(struct kbd *kbd)
{
    int stat;

    /* Enable keyboard*/
    stat = kbd_write_ctrl_cmd(KBD_CTRL_CMD_ENABLE);

    if ( KBD_OK != stat ) {
        kbd->disabled = 1;
    } else {
        kbd->disabled = 0;
    }

    return stat;
}

/*
 * Initialize the keybaord driver
 */
int
kbd_init(struct kbd *kbd)
{
    int stat;

    /* Initialize keyboard state */
    kbd->key_state.caps_on = 0;
    kbd->key_state.alt_on = 0;
    kbd->key_state.shift_on = 0;
    kbd->key_state.ctrl_on = 0;
    kbd->key_state.numlock_on = 0;
    kbd->key_state.scrolllock_on = 0;
    kbd->key_state.insert_on = 0;

    kbd->disabled = 0;

    /* Set LED */
    stat = kbd_set_led(KBD_LED_NONE);

    return stat;
}


/*
 * Keyboard interrupt handler (ring 0...)
 */
void
kbd_intr(void)
{}

/*
 * Send a reset signal via keyboard controller
 */
int
kbd_power_reset(void)
{
    struct sysarch_io io;
    int c;

    /* Empty the keyboard buffer to send reset request */
    do {
        io.port = KBD_CTRL_STAT;
        sysarch(SYSARCH_INB, &io);
        c = io.data;
        if ( 0 != (c & 1) ) {
            io.port = KBD_ENC_BUF;
            sysarch(SYSARCH_INB, &io);
        }
    } while ( 0 != (c & 2) );

    /* CPU reset */
    io.port = KBD_CTRL_CMD;
    io.data = 0xfe;
    sysarch(SYSARCH_OUTB, &io);

    return -1;
}

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    char buf[512];
    struct sysarch_io io;
    struct timespec tm;
    struct kbd kbd;
    unsigned char kbd_scan_code;

    /* Initialize the keyboard driver */
    kbd_init(&kbd);

    /* Print out the interrupt handler */
    driver_register_irq_handler(1, kbd_intr);
    snprintf(buf, 512, "Registered an interrupt handler of %s driver.", "abcd");
    write(STDOUT_FILENO, buf, strlen(buf));

    while ( 1 ) {
        tm.tv_sec = 1;
        tm.tv_nsec = 0;
        nanosleep(&tm, NULL);

        for ( ;; ) {
            io.port = KBD_CTRL_STAT;
            sysarch(SYSARCH_INB, &io);
            if ( io.data & 1 ) {
                kbd_scan_code = kbd_enc_read_buf();
                snprintf(buf, 512, "Input: %x 0x%x.", io.data, kbd_scan_code);
                write(STDOUT_FILENO, buf, strlen(buf));

                if ( kbd_scan_code == 0x01 ) {
                    kbd_power_reset();
                }
            } else {
                break;
            }
        }
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
