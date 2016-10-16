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

#ifndef _KBD_H
#define _KBD_H

#include <stdint.h>
#include <sys/types.h>

#define KBD_KEY_CTRL_LEFT       0x1d
#define KBD_KEY_SHIFT_LEFT      0x2a
#define KBD_KEY_SHIFT_RIGHT     0x36
#define KBD_KEY_CAPS_LOCK       0x3a
#define KBD_KEY_CTRL_RIGHT      0x5a
#define KBD_KEY_UP              0x48
#define KBD_KEY_LEFT            0x4b
#define KBD_KEY_RIGHT           0x4d
#define KBD_KEY_DOWN            0x50

#define KBD_KEY_F1              0x3b
#define KBD_KEY_F2              0x3c
#define KBD_KEY_F3              0x3d
#define KBD_KEY_F4              0x3e
#define KBD_KEY_F5              0x3f
#define KBD_KEY_F6              0x40
#define KBD_KEY_F7              0x41
#define KBD_KEY_F8              0x42
#define KBD_KEY_F9              0x43
#define KBD_KEY_F10             0x44
#define KBD_KEY_F11             0x57
#define KBD_KEY_F12             0x58

#define KBD_ASCII_UP            0x86
#define KBD_ASCII_LEFT          0x83
#define KBD_ASCII_RIGHT         0x84
#define KBD_ASCII_DOWN          0x85

#define KBD_IBUF_SIZE           128

/*
 * Key state
 */
struct kbd_key_state {
    int lctrl:1;
    int rctrl;
    int lshift:1;
    int rshift:1;
    int capslock:1;
    int numlock:1;
    int scrolllock:1;
    int insert:1;
};

/*
 * Keyboard device
 */
struct kbd {
    int disabled;
    struct kbd_key_state key_state;
};

static unsigned char keymap_base[] =
    "  1234567890-=\x08\t"      /* 0x00-0x0f */
    "qwertyuiop[]\r as"         /* 0x10-0x1f */
    "dfghjkl;'` \\zxcv"         /* 0x20-0x2f */
    "bnm,./          "          /* 0x30-0x3f */
    "                "          /* 0x40-0x4f */
    "                "          /* 0x50-0x5f */
    "                "          /* 0x60-0x6f */
    "                ";         /* 0x70-0x7f */

static unsigned char keymap_shift[] =
    "  !@#$%^&*()_+\x08\t"      /* 0x00-0x0f */
    "QWERTYUIOP{}\r AS"         /* 0x10-0x1f */
    "DFGHJKL:\"~ |ZXCV"         /* 0x20-0x2f */
    "BNM<>?          "          /* 0x30-0x3f */
    "                "          /* 0x40-0x4f */
    "                "          /* 0x50-0x5f */
    "                "          /* 0x60-0x6f */
    "                ";         /* 0x70-0x7f */

#endif /* _KBD_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
