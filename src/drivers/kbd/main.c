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
#include <sys/syscall.h>
#include <mki/driver.h>

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
    char buf[512];
    uint16_t *video;
    ssize_t i;

    video = (uint16_t *)0xc00b8000;
    for ( i = 0; i < 80 * 25; i++ ) {
        *(video + i) = 0x0f00;
    }
    snprintf(buf, 512, "kbd %s", "input");
    for ( i = 0; i < (ssize_t)strlen(buf); i++ ) {
        *video = 0x0f00 | (uint16_t)((char *)buf)[i];
        video++;
    }
}

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    char buf[512];

    driver_register_irq_handler(1, kbd_intr);
    snprintf(buf, 512, "Registered an interrupt handler of %s driver.", "abcd");
    write(1, buf, strlen(buf));

    while ( 1 ) {
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
