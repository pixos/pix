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

#include <aos/const.h>
#include <sys/syscall.h>
#include "kernel.h"

/*
 * Initialize the kernel (called from the bootstrap processor initialization
 * sequence)
 */
void
kinit(void)
{
    int i;

    /* Check the kernel variable size */
    if ( sizeof(struct kernel_variables) > KVAR_SIZE ) {
        panic("Invalid struct kernel_variable size.");
    }

    /* Initialize kernel timer */
    g_timer.head = NULL;
    g_jiffies = 0;

    /* Initialize devfs */
    g_devfs.head = NULL;

    /* Setup system calls */
    for ( i = 0; i < SYS_MAXSYSCALL; i++ ) {
        g_syscall_table[i] = NULL;
    }
    g_syscall_table[SYS_exit] = sys_exit;
    g_syscall_table[SYS_fork] = sys_fork;
    g_syscall_table[SYS_read] = sys_read;
    g_syscall_table[SYS_write] = sys_write;
    g_syscall_table[SYS_open] = sys_open;
    g_syscall_table[SYS_close] = sys_close;
    g_syscall_table[SYS_wait4] = sys_wait4;
    g_syscall_table[SYS_getpid] = sys_getpid;
    g_syscall_table[SYS_getuid] = sys_getuid;
    g_syscall_table[SYS_kill] = sys_kill;
    g_syscall_table[SYS_getppid] = sys_getppid;
    g_syscall_table[SYS_getgid] = sys_getgid;
    g_syscall_table[SYS_execve] = sys_execve;
    g_syscall_table[SYS_mmap] = sys_mmap;
    g_syscall_table[SYS_munmap] = sys_munmap;
    g_syscall_table[SYS_lseek] = sys_lseek;
    g_syscall_table[SYS_nanosleep] = sys_nanosleep;
    /* PIX-specific system calls */
    g_syscall_table[SYS_pix_create_jobs] = sys_pix_create_jobs;
    /* Others */
    g_syscall_table[SYS_xpsleep] = sys_xpsleep;
    g_syscall_table[SYS_debug] = sys_debug;
    g_syscall_table[SYS_driver] = sys_driver;
    g_syscall_table[SYS_sysarch] = sys_sysarch;
}


/*
 * Entry point to the kernel in C for all processors, called from asm.s.
 */
void
kmain(void)
{
    for ( ;; ) {
        halt();
    }
}

/*
 * Local APIC timer
 * Low-level scheduler (just loading run queue)
 */
void
isr_loc_tmr(void)
{
    struct ktask *ktask;
    struct ktimer_event *e;
    struct ktimer_event *etmp;

    /* FIXME: This variable should be CPU-specific value (i.e., one variable per
       CPU core). */
    g_jiffies++;

    e = g_timer.head;
    while ( NULL != e && e->jiffies < g_jiffies ) {
        /* Fire the event */
        ktask = e->proc->tasks;
        while ( NULL != ktask ) {
            ktask->state = KTASK_STATE_READY;
            ktask = ktask->proc_task_next;
        }
        etmp = e;
        e = e->next;
        kfree(etmp);
    }
    g_timer.head = e;

    ktask = this_ktask();
    if ( ktask ) {
        /* Decrement the credit */
        ktask->credit--;
        if ( ktask->credit <= 0 ) {
            /* Expires */
            if ( ktask->next ) {
                /* Schedule the next task */
                set_next_ktask(ktask->next);
            } else {
                /* Call high-level scheduler */
                sched_high();
            }
        }
    }
}

/*
 * PIX interprocessor interrupts
 */
void
isr_pixipi(void)
{
    //set_next_ktask(ktask);
}

/*
 * Run an interrupt handler for IRQ interrupt
 */
static void
_irq_handler(u64 vec)
{
    struct ktask *tmp;
    struct interrupt_handler_list *e;

    /* Check whether the interrupt handler is registered */
    if ( NULL != g_intr_table->ivt[vec].handlers ) {

        e = g_intr_table->ivt[vec].handlers;
        while ( NULL != e ) {
            tmp = e->proc->tasks;
            while ( NULL != tmp ) {
                tmp->state = KTASK_STATE_READY;
                tmp->signaled = 0;
                tmp = tmp->proc_task_next;
            }
            e = e->next;
        }
    }
}

/*
 * Interrupt service routine
 */
void
kintr_isr(u64 vec)
{
    switch ( vec ) {
    case IV_LOC_TMR:
        isr_loc_tmr();
        break;
    case IV_PIXIPI:
        isr_pixipi();
        break;
    case IV_IRQ(0):
    case IV_IRQ(1):
    case IV_IRQ(2):
    case IV_IRQ(3):
    case IV_IRQ(4):
    case IV_IRQ(5):
    case IV_IRQ(6):
    case IV_IRQ(7):
    case IV_IRQ(8):
    case IV_IRQ(9):
    case IV_IRQ(10):
    case IV_IRQ(11):
    case IV_IRQ(12):
    case IV_IRQ(13):
    case IV_IRQ(14):
    case IV_IRQ(15):
        _irq_handler(vec);
        break;
    default:
        ;
    }
}

#if !defined(HAS_KMEMSET) || !HAS_KMEMSET
/*
 * kmemset
 */
void *
kmemset(void *b, int c, size_t len)
{
    size_t i;

    i = 0;
    while ( len > 0 ) {
        ((u8 *)b)[i] = c;
        i++;
        len--;
    }

    return b;
}
#endif

#if !defined(HAS_KMEMCMP) || !HAS_KMEMCMP
/*
 * kmemcmp
 */
int
kmemcmp(const void *s1, const void *s2, size_t n)
{
    size_t i;
    int diff;

    i = 0;
    while ( n > 0 ) {
        diff = (u8)((u8 *)s1)[i] - ((u8 *)s2)[i];
        if ( diff ) {
            return diff;
        }
        i++;
        n--;
    }

    return 0;
}
#endif

#if !defined(HAS_KMEMCPY) || !HAS_KMEMCPY
/*
 * kstrcpy
 */
void *
kmemcpy(void *__restrict dst, const void *__restrict src, size_t n)
{
    size_t i;

    for ( i = 0; i < n; i++ ) {
        *((u8 *)dst + i) = *((u8 *)src + i);
    }

    return dst;
}
#endif

/*
 * kstrlen
 */
size_t
kstrlen(const char *s)
{
    size_t len;

    len = 0;
    while ( '\0' != *s ) {
        len++;
        s++;
    }

    return len;
}

/*
 * kstrcmp
 */
int
kstrcmp(const char *s1, const char *s2)
{
    size_t i;
    int diff;

    i = 0;
    while ( s1[i] != '\0' || s2[i] != '\0' ) {
        diff = s1[i] - s2[i];
        if ( diff ) {
            return diff;
        }
        i++;
    }

    return 0;
}

/*
 * kstrncmp
 */
int
kstrncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;
    int diff;

    i = 0;
    while ( (s1[i] != '\0' || s2[i] != '\0') && i < n ) {
        diff = s1[i] - s2[i];
        if ( diff ) {
            return diff;
        }
        i++;
    }

    return 0;
}

/*
 * kstrcpy
 */
char *
kstrcpy(char *dst, const char *src)
{
    size_t i;

    i = 0;
    while ( src[i] != '\0' ) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = src[i];

    return dst;
}

/*
 * kstrncpy
 */
char *
kstrncpy(char *dst, const char *src, size_t n)
{
    size_t i;

    i = 0;
    while ( src[i] != '\0' && i < n ) {
        dst[i] = src[i];
        i++;
    }
    for ( ; i < n; i++ ) {
        dst[i] = '\0';
    }

    return dst;
}

/*
 * kstrlcpy
 */
size_t
kstrlcpy(char *dst, const char *src, size_t n)
{
    size_t i;

    i = 0;
    while ( src[i] != '\0' && i < n - 1 ) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';

    while ( '\0' != src[i] ) {
        i++;
    }

    return i;
}

/*
 * kstrdup
 */
char *
kstrdup(const char *s1)
{
    size_t len;
    char *s;

    len = kstrlen(s1);
    s = kmalloc(len + 1);
    if ( NULL == s ) {
        return NULL;
    }
    kmemcpy(s, s1, len + 1);

    return s;
}

/*
 * kvsscanf
 */
int
kvsscanf(const char *s, const char *format, va_list arg)
{
    return -1;
}

/*
 * ksscanf
 */
int
ksscanf(const char *s, const char *format, ...)
{
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = kvsscanf(s, format, ap);
    va_end(ap);

    return ret;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
