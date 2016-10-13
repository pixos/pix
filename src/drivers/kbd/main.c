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

/*
 * Prototype declarations
 */
int kbd_wait_until_outbuf_full(void);


unsigned char
kbd_read_ctrl_status(void)
{
    struct sysarch_io io;

    io.port = KBD_CTRL_STAT;
    sysarch(SYSARCH_INB, &io);
    return io.data;
}


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
#if 0
    int k;
    int v;

    k = KBD_ENC_BUF;
    __asm__ __volatile__ ("inb %%dx,%%al" : "=a"(v) : "d"(k));
    return v;
#endif
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


#define KBD_ENC_CMD_SETLED      0xed
#define KBD_ENC_CMD_ENABLE      0xF4
#define KBD_ENC_CMD_DISABLE     0xF5

#define KBD_LED_NONE            0x00000000
#define KBD_LED_SCROLLLOCK      0x00000001
#define KBD_LED_NUMLOCK         0x00000002
#define KBD_LED_CAPSLOCK        0x00000004

int
kbd_set_led(int led)
{
    int stat;

    if ( ( KBD_LED_SCROLLLOCK | KBD_LED_NUMLOCK | KBD_LED_CAPSLOCK ) < led ) {
        return KBD_ERROR;
    }

    stat = kbd_enc_write_cmd(KBD_ENC_CMD_SETLED);
    stat |= kbd_enc_write_cmd((unsigned char)led);

    return stat;
}

#define KBD_CTRL_CMD_DISABLE    0xad
#define KBD_CTRL_CMD_ENABLE     0xae

#define KBD_CTRL_CMD_SELFTEST       0xaa

#define KBD_CTRL_STAT_SELFTEST_OK   0x55
#define KBD_CTRL_STAT_SELFTEST_NG   0xfc

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

static int kbd_disabled;

int
kbd_disable(void)
{
    int stat;

    /* Disable keyboard */
    stat = kbd_write_ctrl_cmd(KBD_CTRL_CMD_DISABLE);

    if ( KBD_OK != stat ) {
        kbd_disabled = 0;
    } else {
        kbd_disabled = 1;
    }

    return stat;
}

int
kbd_enable(void)
{
    int stat;

    /* Enable keyboard*/
    stat = kbd_write_ctrl_cmd(KBD_CTRL_CMD_ENABLE);

    if ( KBD_OK != stat ) {
        kbd_disabled = 1;
    } else {
        kbd_disabled = 0;
    }

    return stat;
}

typedef struct {
    unsigned char caps_on;
    unsigned char alt_on;
    unsigned char shift_on;
    unsigned char ctrl_on;
    unsigned char numlock_on;
    unsigned char scrolllock_on;
    unsigned char insert_on;
} kbd_key_state_t;

static kbd_key_state_t kbd_key_state;
static unsigned char kbd_scan_code;


int
kbd_init(void)
{
    int stat;

    /* Initialize keyboard state */
    kbd_key_state.caps_on = 0;
    kbd_key_state.alt_on = 0;
    kbd_key_state.shift_on = 0;
    kbd_key_state.ctrl_on = 0;
    kbd_key_state.numlock_on = 0;
    kbd_key_state.scrolllock_on = 0;
    kbd_key_state.insert_on = 0;

    /* Initialize scan code */
    kbd_scan_code = 0x00;

    /* Set LED */
    stat = kbd_set_led(KBD_LED_NONE);

    return stat;
}



void
sysxpsleep(void)
{
    __asm__ __volatile__ ("syscall" :: "a"(SYS_xpsleep));
}

/*
 * Keyboard interrupt handler (ring 0...)
 */
void
kbd_intr(void)
{
#if 0
    char buf[512];
    uint16_t *video;
    ssize_t i;

    /* Read buffer from keyboard encoder*/
    //kbd_scan_code = kbd_enc_read_buf();

    video = (uint16_t *)0xc00b8000;
    for ( i = 0; i < 80 * 25; i++ ) {
        *(video + i) = 0x0f00;
    }
    snprintf(buf, 512, "kbd input %x", kbd_scan_code);
    for ( i = 0; i < (ssize_t)strlen(buf); i++ ) {
        *video = 0x0f00 | (uint16_t)((char *)buf)[i];
        video++;
    }
#endif
}

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    char buf[512];
    struct sysarch_io io;

    kbd_init();

    driver_register_irq_handler(1, kbd_intr);
    snprintf(buf, 512, "Registered an interrupt handler of %s driver.", "abcd");
    write(1, buf, strlen(buf));

    while ( 1 ) {
        struct timespec tm;
        tm.tv_sec = 1;
        tm.tv_nsec = 0;
        nanosleep(&tm, NULL);

        io.port = KBD_CTRL_STAT;
        sysarch(SYSARCH_INB, &io);
        if ( io.data & 1 ) {
            kbd_scan_code = kbd_enc_read_buf();
            snprintf(buf, 512, "Input: %x 0x%x.", io.data, kbd_scan_code);
            write(1, buf, strlen(buf));
        }
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
