/* -*- Mode: asm -*- */
/*_
 * Copyright (c) 2015-2016 Hirochika Asai <asai@jar.jp>
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

/* Pseudo interrupt descriptor table */
idtr:
	.word	0x0
	.long	0x0

/* Global descriptor table */
gdt:
	.word	0x0,0x0,0x0,0x0		/* Null entry */
	.word	0xffff,0x0,0x9a00,0xaf	/* Code64 */
	.word	0xffff,0x0,0x9200,0xaf	/* Data64 */
	.word	0xffff,0x0,0x9a00,0xcf	/* Code32 */
	.word	0xffff,0x0,0x9200,0xcf	/* Data32 */
	.word	0xffff,0x0,0x9a00,0x8f	/* Code16 */

gdt.1:
	/* Global descriptor table register */
gdtr:
	.word	gdt.1 - gdt - 1		/* Limit */
	.long	gdt			/* Address */
