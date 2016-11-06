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

#ifndef _KERNEL_ARCH_CONST_H
#define _KERNEL_ARCH_CONST_H

/* GDT selectors */
#define GDT_NR                  7
#define GDT_NULL_SEL            (0 << 3)
#define GDT_RING0_CODE_SEL      (1 << 3)
#define GDT_RING0_DATA_SEL      (2 << 3)
#define GDT_RING3_CODE32_SEL    (3 << 3)
#define GDT_RING3_DATA32_SEL    (4 << 3)
#define GDT_RING3_CODE64_SEL    (5 << 3)
#define GDT_RING3_DATA64_SEL    (6 << 3)
#define GDT_TSS_SEL_BASE        (7 << 3)

/* Temporary GDT for application processors */
#define AP_GDT_CODE64_SEL       0x08    /* Code64 selector */
#define AP_GDT_DATA64_SEL       0x10    /* Data selector */
#define AP_GDT_CODE32_SEL       0x18    /* Code32 selector */
#define AP_GDT_DATA32_SEL       0x20    /* Data selector */
#define AP_GDT_CODE16_SEL       0x28    /* Code16 selector */

/* # of interrupts */
#define IDT_NR  256

/* Relocation base */
#define KERNEL_RELOCBASE        0xc0000000

/* Kernel page table (physical address) */
#define KERNEL_PGT              0x00079000
/* Pointers to the system call table (virtual address) */
#define SYSCALL_TABLE           0xc0087000 /* FIXME */
#define SYSCALL_NR              0xc0087010 /* FIXME */
/* Per-processor information (flags, cpuinfo, stats, tss, task, stack) */
#define CPU_DATA_BASE           0xc1000000
#define CPU_DATA_SIZE           0x10000
#define CPU_STACK_GUARD         0x10
#define CPU_TSS_SIZE            104 /* sizeof(struct tss) */
#define CPU_TSS_OFFSET          (0x30 + IDT_NR * 8) /* struct tss */
#define CPU_CUR_TASK_OFFSET     (CPU_TSS_OFFSET + CPU_TSS_SIZE) /* cur_task */
#define CPU_NEXT_TASK_OFFSET    (CPU_CUR_TASK_OFFSET + 8)      /* next_task */
#define CPU_IDLE_TASK_OFFSET    (CPU_NEXT_TASK_OFFSET + 8)     /* idle task */
/* Task information (struct arch_task) */
#define TASK_RP     0
#define TASK_SP0    8
#define TASK_CR3    16
#define TASK_XREGS  24
/* TSS */
#define TSS_SP0     4


/* Trampoline: 0x70 (0x70000) */
#define TRAMPOLINE_VEC          0x90
#define TRAMPOLINE_MAX_SIZE     0x1000

/* Syscall */
//#define SYSCALL_MAX_NR 0x10

#endif /* _KERNEL_ARCH_CONST_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
