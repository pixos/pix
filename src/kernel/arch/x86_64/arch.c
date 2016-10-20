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
#include "../../kernel.h"
#include "arch.h"
#include "desc.h"
#include "acpi.h"
#include "i8254.h"
#include "apic.h"
#include "memory.h"

/* Prototype declarations */
static int load_trampoline(void);
static void cpu_init(void);

/* ACPI structure */
struct acpi arch_acpi;

/* Multiprocessor enabled */
int mp_enabled;


/*
 * Relocate the trampoline code to a 4 KiB page alined space
 */
static int
load_trampoline(void)
{
    int i;
    int tsz;

    /* Check and copy trampoline code */
    tsz = (u64)trampoline_end - (u64)trampoline;

    if ( tsz > TRAMPOLINE_MAX_SIZE ) {
        /* Error when the size of the trampoline code exceeds 4 KiB */
        return -1;
    }
    /* Copy the trampoline code to the trampoline region */
    for ( i = 0; i < tsz; i++ ) {
        *(u8 *)((u64)(TRAMPOLINE_VEC << 12) + i) = *(u8 *)((u64)trampoline + i);
    }

    return 0;
}

/*
 * Initialize CPU data structure
 */
static void
cpu_init(void)
{
    int i;

    /* Reset all processors */
    for ( i = 0; i < MAX_PROCESSORS; i++ ) {
        /* Fill the processor data space with zero excluding stack area */
        kmemset((u8 *)((u64)CPU_DATA_BASE + i * CPU_DATA_SIZE), 0,
                sizeof(struct cpu_data));
    }
}

/*
 * Set up the exception/interrupt handlers
 */
static void
intr_setup(void)
{
    /* Register exceptions as traps */
    idt_setup_trap_gate(0, intr_dze);
    idt_setup_trap_gate(1, intr_debug);
    idt_setup_trap_gate(2, intr_nmi);
    idt_setup_trap_gate(3, intr_breakpoint);
    idt_setup_trap_gate(4, intr_overflow);
    idt_setup_trap_gate(5, intr_bre);
    idt_setup_trap_gate(6, intr_iof);
    idt_setup_trap_gate(7, intr_dna);
    idt_setup_trap_gate(8, intr_df);
    idt_setup_trap_gate(9, intr_cso);
    idt_setup_trap_gate(10, intr_invtss);
    idt_setup_trap_gate(11, intr_snpf);
    idt_setup_trap_gate(12, intr_ssf);
    idt_setup_trap_gate(13, intr_gpf);
    idt_setup_trap_gate(14, intr_pf);
    idt_setup_trap_gate(16, intr_x87_fpe);
    idt_setup_trap_gate(17, intr_acf);
    idt_setup_trap_gate(18, intr_mca);
    idt_setup_trap_gate(19, intr_simd_fpe);
    idt_setup_trap_gate(20, intr_vef);
    idt_setup_trap_gate(30, intr_se);
    /* Interrupts */
    idt_setup_intr_gate(IV_LOC_TMR, intr_apic_loc_tmr);
    idt_setup_intr_gate(IV_LOC_TMR_XP, intr_apic_loc_tmr_xp);
    idt_setup_intr_gate(IV_CRASH, intr_crash);

    /* IRQs */
    idt_setup_intr_gate(IV_IRQ(0), intr_driver_0x20);
    idt_setup_intr_gate(IV_IRQ(1), intr_driver_0x21);
    idt_setup_intr_gate(IV_IRQ(2), intr_driver_0x22);
    idt_setup_intr_gate(IV_IRQ(3), intr_driver_0x23);
    idt_setup_intr_gate(IV_IRQ(4), intr_driver_0x24);
    idt_setup_intr_gate(IV_IRQ(5), intr_driver_0x25);
    idt_setup_intr_gate(IV_IRQ(6), intr_driver_0x26);
    idt_setup_intr_gate(IV_IRQ(7), intr_driver_0x27);
    idt_setup_intr_gate(IV_IRQ(8), intr_driver_0x28);
    idt_setup_intr_gate(IV_IRQ(9), intr_driver_0x29);
    idt_setup_intr_gate(IV_IRQ(10), intr_driver_0x2a);
    idt_setup_intr_gate(IV_IRQ(11), intr_driver_0x2b);
    idt_setup_intr_gate(IV_IRQ(12), intr_driver_0x2c);
    idt_setup_intr_gate(IV_IRQ(13), intr_driver_0x2d);
    idt_setup_intr_gate(IV_IRQ(14), intr_driver_0x2e);
    idt_setup_intr_gate(IV_IRQ(15), intr_driver_0x2f);

    /* For driver use */
    idt_setup_intr_gate(0x50, intr_driver_0x50);
    idt_setup_intr_gate(0x51, intr_driver_0x51);
    idt_setup_intr_gate(0x52, intr_driver_0x52);
    idt_setup_intr_gate(0x53, intr_driver_0x53);
    idt_setup_intr_gate(0x54, intr_driver_0x54);
    idt_setup_intr_gate(0x55, intr_driver_0x55);
    idt_setup_intr_gate(0x56, intr_driver_0x56);
    idt_setup_intr_gate(0x57, intr_driver_0x57);
    idt_setup_intr_gate(0x58, intr_driver_0x58);
    idt_setup_intr_gate(0x59, intr_driver_0x59);
    idt_setup_intr_gate(0x5a, intr_driver_0x5a);
    idt_setup_intr_gate(0x5b, intr_driver_0x5b);
    idt_setup_intr_gate(0x5c, intr_driver_0x5c);
    idt_setup_intr_gate(0x5d, intr_driver_0x5d);
    idt_setup_intr_gate(0x5e, intr_driver_0x5e);
    idt_setup_intr_gate(0x5f, intr_driver_0x5f);
}

/*
 * Panic -- damn blue screen, lovely green screen
 */
void
panic(const char *s)
{
    int i;
    u16 *video;
    u16 val;
    int col;
    int ln;

    /* Disable interrupt */
    cli();

    if ( mp_enabled ) {
        /* Notify other processors to stop */
        /* Send IPI and halt self */
        lapic_send_fixed_ipi(IV_CRASH);
    }

    /* Print out the message string directly */
    video = (u16 *)VIDEO_COLOR;

    /* Fill out with green */
    for ( i = 0; i < 80 * 25; i++ ) {
        video[i] = 0x2f00;
    }

    col = 0;
    ln = 0;
    for ( i = 0; *s; s++  ) {
        switch ( *s ) {
        case '\r':
            video -= col;
            i -= col;
            col = 0;
            break;
        case '\n':
            video += 80;
            i += 80;
            ln++;
            break;
        default:
            *video = 0x2f00 | *s;
            video++;
            i++;
            col++;
        }
    }

    /* Move the cursor */
    val = ((i & 0xff) << 8) | 0x0f;
    outw(0x3d4, val);   /* Low */
    val = (((i >> 8) & 0xff) << 8) | 0x0e;
    outw(0x3d4, val);   /* High */

    /* Stop forever */
    while ( 1 ) {
        halt();
    }
}

/*
 * Initialize the bootstrap processor
 */
void
bsp_init(void)
{
    struct bootinfo *bi;
    struct cpu_data *pdata;
    long long i;
    int prox;
    int ret;

    /* Reset */
    mp_enabled = 0;

    /* Ensure the i8254 timer is stopped */
    i8254_stop_timer();

    /* Boot information from the boot monitor */
    bi = (struct bootinfo *)BOOTINFO_BASE;

    /* Reset all processors */
    cpu_init();

    /* Load trampoline code with the boot strap page table */
    ret = load_trampoline();
    if ( ret < 0 ) {
        panic("Fatal: Could not load trampoline");
        return;
    }

    /* Initialize global descriptor table (GDT) */
    gdt_init();

    /* Initialize interrupt descriptor table (IDT) */
    idt_init();

    /* Load GDT/IDT */
    gdt_load();
    idt_load();

    /* Load ACPI */
    kmemset(&arch_acpi, 0, sizeof(struct acpi));
    acpi_load(&arch_acpi);

    /* ToDo: Prepare the virtual pages for ACPI etc. */

    /* Initialize I/O APIC */
    ioapic_init();

    /* Set up interrupt vector */
    intr_setup();

    /* Setup interrupt service routine */
    for ( i = 0; i < 16; i++ ) {
        ioapic_map_intr(IV_IRQ(i), i, arch_acpi.acpi_ioapic_base); /* IRQn */
    }

    /* Get the proximity domain */
    prox = acpi_lapic_prox_domain(&arch_acpi, lapic_id());

    /* Initialize the physical memory manager */
    if ( arch_memory_init(bi, &arch_acpi) < 0 ) {
        panic("Fatal: Could not initialize the memory manager.");
        return;
    }

    /* Load LDT */
    lldt(0);

    /* Initialize TSS */
    tss_init();
    tr_load(lapic_id());

    /* Initialize the process table */
    g_proc_table = kmalloc(sizeof(struct proc_table));
    if ( NULL == g_proc_table ) {
        panic("Fatal: Could not initialize the process table.");
        return;
    }
    for ( i = 0; i < PROC_NR; i++ ) {
        g_proc_table->procs[i] = NULL;
    }
    g_proc_table->lastpid = -1;

    /* Set up the interrupt handler table */
    g_intr_table = kmalloc(sizeof(struct interrupt_handler_table));
    if ( NULL == g_intr_table ) {
        panic("Fatal: Could not initialize the interrupt handler table.");
        return;
    }
    for ( i = 0; i < NR_IV; i++ ) {
        g_intr_table->ivt[i].handlers = NULL;
    }

    /* Initialize the task lists */
    g_ktask_root = kmalloc(sizeof(struct ktask_root));
    if ( NULL == g_ktask_root ) {
        panic("Fatal: Could not initialize the task lists.");
        return;
    }
    g_ktask_root->r.head = NULL;
    g_ktask_root->r.tail = NULL;
    g_ktask_root->b.head = NULL;
    g_ktask_root->b.tail = NULL;

    /* Enable this processor */
    pdata = this_cpu();
    pdata->cpu_id = lapic_id();
    pdata->prox_domain = prox;
    pdata->flags |= 1;

    /* Estimate the frequency */
    pdata->freq = lapic_estimate_freq();

    /* Set an idle task for this processor */
    pdata->idle_task = task_create_idle();
    if ( NULL == pdata->idle_task ) {
        panic("Fatal: Could not create the idle task for BSP.");
        return;
    }

    /* Initialize initramfs */
    if ( ramfs_init((u64 *)KMEM_P2V(INITRAMFS_BASE)) < 0 ) {
        panic("Fatal: Could not initialize the ramfs.");
        return;
    }

    /* Initialize the kernel */
    kinit();

    void *test = kmalloc(64);
    kfree(test);

#if 0
    if ( vmx_enable() < 0 ) {
        panic("Failed to initialize VMX.");
        return;
    }
    if ( vmx_initialize_vmcs() ) {
        panic("Failed on VMCX initialization.");
        return;
    }
    if ( vmlaunch() ) {
        panic("Failed on VMLAUNCH.");
        return;
    }
#endif

    /* Enable MP */
    mp_enabled = 1;

    /* Send INIT IPI */
    lapic_send_init_ipi();

    /* Wait 10 ms */
    acpi_busy_usleep(&arch_acpi, 10000);

    /* Send a Start Up IPI */
    lapic_send_startup_ipi(TRAMPOLINE_VEC & 0xff);

    /* Wait 200 us */
    acpi_busy_usleep(&arch_acpi, 200);

    /* Send another Start Up IPI */
    lapic_send_startup_ipi(TRAMPOLINE_VEC & 0xff);

    /* Wait 200 us */
    acpi_busy_usleep(&arch_acpi, 200);

    /* Initialize local APIC counter */
    lapic_start_timer(HZ, IV_LOC_TMR);

    /* Launch the `init' server */
    cli();

    if ( proc_create("/servers/pm", "pm", 0) < 0 ) {
        panic("Fatal: Cannot create the `pm' server.");
        return;
    }
    if ( proc_create("/servers/init", "init", 1) < 0 ) {
        panic("Fatal: Cannot create the `init' server.");
        return;
    }

    /* Schedule the idle task */
    this_cpu()->cur_task = NULL;
    this_cpu()->next_task = this_cpu()->idle_task;

    set_next_ktask(g_ktask_root->r.head->ktask);

    /* Start the idle task */
    task_restart();
}

/*
 * Initialize the application processor
 */
void
ap_init(void)
{
    struct cpu_data *pdata;
    int prox;

    /* Load global descriptor table */
    gdt_load();

    /* Load interrupt descriptor table */
    idt_load();

    /* Get the proximity domain */
    prox = acpi_lapic_prox_domain(&arch_acpi, lapic_id());

    /* Disable the global page feature */
    set_cr4(get_cr4() & ~CR4_PGE);
    /* Set the page table */
    set_cr3(((struct arch_kmem_space *)g_kmem->space->arch)->cr3);
    /* Enable the global page feature */
    set_cr4(get_cr4() | CR4_PGE);

    /* Enable this processor */
    pdata = this_cpu();
    pdata->cpu_id = lapic_id();
    pdata->prox_domain = prox;
    pdata->flags |= 1;

    /* Estimate the frequency */
    pdata->freq = lapic_estimate_freq();

    /* Set an idle task for this processor */
    pdata->idle_task = task_create_idle();
    if ( NULL == pdata->idle_task ) {
        panic("Fatal: Could not create the idle task for AP.");
        return;
    }

    /* Load LDT */
    lldt(0);

    /* Load TSS */
    tr_load(lapic_id());
}

/*
 * Get the CPU data structure
 */
struct cpu_data *
this_cpu(void)
{
    return (struct cpu_data *)((u64)CPU_DATA_BASE + lapic_id() * CPU_DATA_SIZE);
}

/*
 * Execute a process
 */
int
arch_exec(struct arch_task *t, void (*entry)(void), size_t size, int policy,
          char *const argv[], char *const envp[])
{
    int argc;
    char *const *tmp;
    size_t len;
    void *ustack;
    u8 *arg;
    char **narg;
    u8 *saved;
    u64 cs;
    u64 ss;
    u64 flags;

    /* Set the user stack address */
    ustack = t->ustack;

    /* Count the number of arguments */
    tmp = argv;
    argc = 0;
    len = 0;
    while ( NULL != *tmp ) {
        argc++;
        len += kstrlen(*tmp);
        tmp++;
    }

    /* Prepare arguments from stack */
    len += argc + (argc + 1) * sizeof(void *);
    arg = ustack;

    tmp = argv;
    narg = (char **)arg;
    saved = arg + sizeof(void *) * (argc + 1);
    while ( NULL != *tmp ) {
        *narg = (char *)(saved);
        kmemcpy(saved, *tmp, kstrlen(*tmp));
        saved[kstrlen(*tmp)] = '\0';
        saved += kstrlen(*tmp) + 1;
        tmp++;
        narg++;
    }
    *narg = NULL;

    /* Configure the ring protection by the policy */
    switch ( policy ) {
    case KTASK_POLICY_KERNEL:
        cs = GDT_RING0_CODE_SEL;
        ss = GDT_RING0_DATA_SEL;
        flags = 0x0200;
        break;
    case KTASK_POLICY_DRIVER:
    case KTASK_POLICY_SERVER:
    case KTASK_POLICY_USER:
    default:
        cs = GDT_RING3_CODE64_SEL + 3;
        ss = GDT_RING3_DATA64_SEL + 3;
        flags = 0x3200;
        break;
    }

    /* For exec */
    void *paddr;
    paddr = pmem_prim_alloc_pages(PMEM_ZONE_LOWMEM,
                                  bitwidth(DIV_CEIL(size, SUPERPAGESIZE)));
    if ( NULL == paddr ) {
        return -1;
    }
    ssize_t i;
    int ret;
    for ( i = 0; i < (ssize_t)DIV_CEIL(size, SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(t->ktask->proc->vmem,
                            (void *)(CODE_INIT + SUPERPAGESIZE * i),
                            paddr + SUPERPAGESIZE * i,
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            return -1;
        }
    }

    /* Release the original one */
    pmem_prim_free_pages(t->ktask->proc->code_paddr);
    t->ktask->proc->code_paddr = paddr;

    kmemcpy((void *)CODE_INIT, entry, size);
    kmemset(t->rp, 0, sizeof(struct stackframe64));
    /* Replace the current process with the new process */
    t->sp0 = (u64)t->kstack + KSTACK_SIZE - 16;
    t->rp->gs = ss;
    t->rp->fs = ss;
    t->rp->sp = USTACK_INIT + USTACK_SIZE - 16 - 8;
    t->rp->ss = ss;
    t->rp->cs = cs;
    t->rp->ip = CODE_INIT;
    t->rp->flags = flags;

    t->rp->di = argc;
    t->rp->si = USTACK_INIT;

    /* Specify the code size */
    t->ktask->proc->code_size = size;

    /* Restart the task */
    task_replace(t);

    /* Never reach here but do this to prevent a compiler error */
    return -1;
}

/*
 * A routine called when task is switched
 * Note that this is in the interrupt handler and DO NOT change the interrupt
 * flag (i.e., DO NOT use sti/cli).  Also use caution in use of lock.
 */
void
arch_task_switched(struct arch_task *prev, struct arch_task *next)
{
}

void
arch_switch_page_table(struct vmem_space *vmem)
{
    static void *cr3;
    struct arch_vmem_space *avmem;

    if ( NULL == vmem ) {
        set_cr3(cr3);
    } else {
        cr3 = get_cr3();
        avmem = (struct arch_vmem_space *)vmem->arch;
        set_cr3(avmem->pgt);
    }
}

/*
 * Get the kernel task currently running on this processor
 */
struct ktask *
this_ktask(void)
{
    struct cpu_data *pdata;

    /* Get the information on this processor */
    pdata = this_cpu();
    if ( NULL == pdata->cur_task ) {
        /* No task running on this processor */
        return NULL;
    }

    /* Return the kernel task data structure */
    return pdata->cur_task->ktask;
}

/*
 * Schedule the next task
 */
void
set_next_ktask(struct ktask *ktask)
{
    this_cpu()->next_task = ktask->arch;
}

/*
 * Schedule the idle task as the next
 */
void set_next_idle(void)
{
    this_cpu()->idle_task->ktask->credit = 0;
    this_cpu()->next_task = this_cpu()->idle_task;
}

/*
 * Idle task
 */
void
arch_idle(void)
{
    while ( 1 ) {
        halt();
    }
}

/*
 * Exception handler
 */
void
isr_exception(int nr, void *ip, u64 cs, u64 flags, void *sp)
{
    char buf[128];

    ksnprintf(buf, sizeof(buf), "#%d: ip=%llx, cs=%llx, flags=%llx, sp=%llx",
              nr, ip, cs, flags, sp);
    panic(buf);
}

/*
 * Exception handler with error code
 */
void
isr_exception_werror(int nr, u64 error, void *ip, u64 cs, u64 flags, void *sp)
{
    char buf[128];

    ksnprintf(buf, sizeof(buf), "#%d (%llx): ip=%llx, cs=%llx, flags=%llx, "
              "sp=%llx", nr, error, ip, cs, flags, sp);
    panic(buf);
}

/*
 * Debug fault/trap
 */
void
isr_debug(void *ip, u64 cs, u64 flags, void *sp, u64 ss)
{
    char *buf = this_ktask()->proc->name;
    panic(buf);
}

/*
 * Page fault handler
 */
void
isr_page_fault(void *addr, u64 error, void *rip, u64 cs, u64 flags, void *sp)
{
    char buf[128];
    u64 x = (u64)rip;
    u64 y = (u64)addr;
    struct ktask *t;

    /* Get the current process */
    t = this_ktask();
    if ( NULL == t || NULL == t->proc ) {
        ksnprintf(buf, sizeof(buf),
                  "An unknown task causes page fault: %016llx @%016llx", y,
                  x);
        panic(buf);
        return;
    }

    ksnprintf(buf, sizeof(buf), "Page Fault (%c%c%c%c[%d]): %016x @%016x %d",
              (error & 0x10) ? 'I' : 'D', (error & 0x4) ? 'U' : 'S',
              (error & 0x2) ? 'W' : 'R', (error & 0x1) ? 'P' : '*',
              error, y, x,
              (NULL != t && NULL != t->proc) ? t->proc->id : -1);
    panic(buf);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
