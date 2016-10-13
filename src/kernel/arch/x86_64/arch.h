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

#ifndef _KERNEL_ARCH_H
#define _KERNEL_ARCH_H

#include <aos/const.h>
#include <aos/types.h>
#include "const.h"

/* Color video RAM (virtual memory) */
#define VIDEO_COLOR             0xc00b8000ULL

/* Lowest memory address managed by memory allocator
 * Note that ISA memory hole (0x00f00000-0x00ffffff) are detected as available
 * in the system memory map obtained from the BIOS, so be carefull if we use
 * the address below 0x01000000 for PMEM_LBOUND.  The range from 0x01000000 to
 * 0x01ffffff is used by the management data structure of processors.
 */
#define PMEM_LBOUND             0x02000000ULL
/* Highest memory address mapped to the linear address of the boot strap page
   table */
#define PMEM_MAPPED_UBOUND      0xbfffffffULL

/* Physical memory address used by the kernel memory allocator */
#define KMEM_BASE               0x00100000ULL
#define KMEM_MAX_SIZE           0x00e00000ULL


#define KERNEL_BASE             0xc0000000ULL
#define KERNEL_SIZE             0x18000000ULL
#define KERNEL_SPEC_SIZE        0x28000000ULL

#define KMEM_P2V(a)             ((u64)(a) + KERNEL_BASE)
#define KMEM_LOW_P2V(a)         ((u64)(a))

#define KMEM_REGION_KERNEL_BASE 0xc0000000ULL
#define KMEM_REGION_KERNEL_SIZE 0x10000000ULL

#define KMEM_REGION_SPEC_BASE   0xd0000000ULL
#define KMEM_REGION_SPEC_SIZE   0x30000000ULL

#define KMEM_REGION_PMEM_BASE   0x100000000ULL


/* Maximum number of processors supported in this operating system */
#define MAX_PROCESSORS          256

/* GDT and IDT */
#define GDT_ADDR                0xc0080000ULL
#define GDT_MAX_SIZE            0x2000
#define IDT_ADDR                0xc0082000ULL
#define IDT_MAX_SIZE            0x2000


/* Control registers */
#define CR0_PE                  (1ULL << 0) /* Protection Enable */
#define CR0_MP                  (1ULL << 1) /* Monitor Coprocessor */
#define CR0_EM                  (1ULL << 2) /* Emulation */
#define CR0_TS                  (1ULL << 3) /* Task Switched */
#define CR0_ET                  (1ULL << 4) /* Extention Type */
#define CR0_NE                  (1ULL << 5) /* Numeric Error */
#define CR0_WP                  (1ULL << 16) /* Write Protect */
#define CR0_AM                  (1ULL << 18) /* Alignment Mask */
#define CR0_NW                  (1ULL << 29) /* Not Write-through */
#define CR0_CD                  (1ULL << 30) /* Cache Disable */
#define CR0_PG                  (1ULL << 31) /* Paging */

#define CR4_VME                 (1ULL << 0)
#define CR4_PVI                 (1ULL << 1)
#define CR4_TSD                 (1ULL << 2)
#define CR4_DE                  (1ULL << 3)
#define CR4_PSE                 (1ULL << 4)
#define CR4_PAE                 (1ULL << 5)
#define CR4_MCE                 (1ULL << 6)
#define CR4_PGE                 (1ULL << 7)
#define CR4_PCE                 (1ULL << 8)
#define CR4_OSFXSR              (1ULL << 9)
#define CR4_OSXMMEXCPT          (1ULL << 10)
#define CR4_VMXE                (1ULL << 13)
#define CR4_SMXE                (1ULL << 14)
#define CR4_FSGSBASE            (1ULL << 16)
#define CR4_PCIDE               (1ULL << 17)
#define CR4_OSXSAVE             (1ULL << 18)
#define CR4_SMEP                (1ULL << 20)

/*
 * Boot information from boot loader
 */
struct bootinfo {
    /* Systen address map obtained through BIOS */
    struct {
        u64 nr;
        struct bootinfo_sysaddrmap_entry *entries;      /* u64 */
    } __attribute__ ((packed)) sysaddrmap;
} __attribute__ ((packed));
struct bootinfo_sysaddrmap_entry {
    u64 base;
    u64 len;
    u32 type;
    u32 attr;
} __attribute__ ((packed));

/*
 * Stack frame for interrupt handlers
 */
struct stackframe64 {
    /* Segment registers */
    u16 gs;
    u16 fs;

    /* Base pointer */
    u64 bp;

    /* Index registers */
    u64 di;
    u64 si;

    /* Generic registers */
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 dx;
    u64 cx;
    u64 bx;
    u64 ax;

    /* Restored by `iretq' instruction */
    u64 ip;             /* Instruction pointer */
    u64 cs;             /* Code segment */
    u64 flags;          /* Flags */
    u64 sp;             /* Stack pointer */
    u64 ss;             /* Stack segment */
} __attribute__ ((packed));

/*
 * TSS
 */
struct tss {
    u32 reserved1;
    u32 rsp0l;
    u32 rsp0h;
    u32 rsp1l;
    u32 rsp1h;
    u32 rsp2l;
    u32 rsp2h;
    u32 reserved2;
    u32 reserved3;
    u32 ist1l;
    u32 ist1h;
    u32 ist2l;
    u32 ist2h;
    u32 ist3l;
    u32 ist3h;
    u32 ist4l;
    u32 ist4h;
    u32 ist5l;
    u32 ist5h;
    u32 ist6l;
    u32 ist6h;
    u32 ist7l;
    u32 ist7h;
    u32 reserved4;
    u32 reserved5;
    u16 reserved6;
    u16 iomap;
} __attribute__ ((packed));

/*
 * Page table entry (4 KiB block)
 */
struct page_entry {
    u64 entries[512];
} __attribute__ ((packed));


/*
 * Architecture specific page entry
 */
struct arch_page_entry {
    union {
        struct arch_page_dir *child;
        void *addr;
        u64 bits;
    } u;
};
/*
 * Page directory
 */
struct arch_page_dir {
    u64 entries[512];
};

/*
 * Kernel memory space (i.e., page table).  In this architecture, the kernel
 * memory region is mapped from 3 to 4 GiB.
 */
struct arch_kmem_space {
    /* The root of the 4-level page table (virtual address); the physical
       address of the next member variable, pdpt, is set to pml4->entries[0]
       with flags (i.e., for the region of 0-512 GiB). */
    struct arch_page_dir *pml4;
    /* The pointer to page directory pointer table; the physical address of the
       next member variable, pd, is set to pdpt->entries[3] with flags (i.e.,
       for the region of 3-4 GiB). */
    struct arch_page_dir *pdpt;
    /* The pointer to the page directory */
    struct arch_page_dir *pd;

    /* cr3 (physical address) */
    void *cr3;
};

/*
 * Level-3 Complete multiway (512-ary) tree for virtual memory
 */
#define VMEM_NENT(x)        (DIV_CEIL((x), 512 * 512) + DIV_CEIL((x), 512) \
                             + (x))
#define VMEM_PML4(x)        (x[0])
#define VMEM_PDPT(x, pg)    (x[0 + DIV_CEIL((pg) + 1, 512) + FLOOR((pg), 512)])
#define VMEM_PD(x, pg)      (x[1 + DIV_CEIL((pg) + 1, 512) + pg])
struct arch_vmem_space {
    /* The root of the page table */
    void *pgt;
    /* Level-3 complete multiway (512-ary) tree */
    int nr;
    u64 **array;
    /* Leaves for virtual memory */
    u64 **vls;

#if 0
    struct {
        /* The root of the 4-level page table (virtual address) */
        struct arch_page_dir *pml4;
    } virt;
    struct {
        struct arch_page_dir *pml4;
        struct arch_page_dir **pdpt;
        struct arch_page_dir **pd;
    } phys;

    /* cr3 (physical address) */
    void *cr3;
#endif
};

/*
 * Task (architecture specific structure)
 */
struct arch_task {
    /* Do not change the first four.  These must be on the top.  See asm.S and
       const.h. */
    /* Restart point (a part of kstack) */
    struct stackframe64 *rp;
    /* SP0 for tss */
    u64 sp0;
    /* CR3: Physical address of the page table */
    void *cr3;
    /* FPU/SSE registers */
    void *xregs;
    /* Kernel stack pointer (kernel address) */
    void *kstack;
    /* User stack pointer (virtual address) */
    void *ustack;
    /* Parent structure (architecture-independent generic task structure) */
    struct ktask *ktask;
} __attribute__ ((packed));


/*
 * Data space for each processor
 */
struct cpu_data {
    u32 flags;          /* bit 0: enabled (working); bit 1- reserved */
    u32 cpu_id;
    u64 freq;           /* Frequency */
    u32 prox_domain;
    u32 reserved[3];
    u64 stats[IDT_NR];  /* Interrupt counter */
    /* CPU_TSS_OFFSET */
    struct tss tss;
    /* CPU_CUR_TASK_OFFSET */
    struct arch_task *cur_task;
    /* CPU_NEXT_TASK_OFFSET */
    struct arch_task *next_task;
    /* Idle task (CPU_IDLE_TASK_OFFSET) */
    struct arch_task *idle_task;
    /* Stack and stack guard follow */
} __attribute__ ((packed));

/* in arch.c */
struct cpu_data * this_cpu(void);
int
arch_exec(struct arch_task *, void (*)(void), size_t, int, char *const [],
          char *const []);
void arch_idle(void);

/* in vmx.c */
int vmx_enable(void);
int vmx_initialize_vmcs(void);

/* in asm.s */
void lidt(void *);
void lgdt(void *, u64);
void sidt(void *);
void sgdt(void *);
void lldt(u16);
void ltr(u16);
void cli(void);
void sti(void);
void intr_null(void);

void intr_dze(void);
void intr_debug(void);
void intr_nmi(void);
void intr_breakpoint(void);
void intr_overflow(void);
void intr_bre(void);
void intr_iof(void);
void intr_dna(void);
void intr_df(void);
void intr_cso(void);
void intr_invtss(void);
void intr_snpf(void);
void intr_ssf(void);
void intr_gpf(void);
void intr_pf(void);
void intr_x87_fpe(void);
void intr_acf(void);
void intr_mca(void);
void intr_simd_fpe(void);
void intr_vef(void);
void intr_se(void);

void intr_driver_0x20(void);
void intr_driver_0x21(void);
void intr_driver_0x22(void);
void intr_driver_0x23(void);
void intr_driver_0x24(void);
void intr_driver_0x25(void);
void intr_driver_0x26(void);
void intr_driver_0x27(void);
void intr_driver_0x28(void);
void intr_driver_0x29(void);
void intr_driver_0x2a(void);
void intr_driver_0x2b(void);
void intr_driver_0x2c(void);
void intr_driver_0x2d(void);
void intr_driver_0x2e(void);
void intr_driver_0x2f(void);

void intr_driver_0x50(void);
void intr_driver_0x51(void);
void intr_driver_0x52(void);
void intr_driver_0x53(void);
void intr_driver_0x54(void);
void intr_driver_0x55(void);
void intr_driver_0x56(void);
void intr_driver_0x57(void);
void intr_driver_0x58(void);
void intr_driver_0x59(void);
void intr_driver_0x5a(void);
void intr_driver_0x5b(void);
void intr_driver_0x5c(void);
void intr_driver_0x5d(void);
void intr_driver_0x5e(void);
void intr_driver_0x5f(void);



void intr_apic_loc_tmr(void);
void intr_apic_loc_tmr_xp(void);
void intr_crash(void);
void task_restart(void);
void task_replace(void *);
void pause(void);
u8 inb(u16);
u16 inw(u16);
u32 inl(u16);
void outb(u16, u8);
void outw(u16, u16);
void outl(u16, u32);
u32 mfread32(u64);
void mfwrite32(u64, u32);
u64 cpuid(u64, u64 *, u64 *);
u64 rdmsr(u64);
void wrmsr(u64, u64);
u64 get_cr0(void);
void set_cr0(u64);
void * get_cr3(void);
void set_cr3(void *);
u64 get_cr4(void);
void set_cr4(u64);
void invlpg(void *);
int vmxon(void *);
int vmclear(void *);
int vmptrld(void *);
int vmwrite(u64, u64);
u64 vmread(u64);
int vmlaunch(void);
int vmresume(void);
void spin_lock_intr(u32 *);
void spin_unlock_intr(u32 *);

/* in trampoline.s */
void trampoline(void);
void trampoline_end(void);

/* in task.c */
struct arch_task * task_create_idle(void);
int proc_create(const char *, const char *, pid_t);

/* In-line assembly */
/* void set_cr0(u64 cr0) */
#define set_cr0(cr0)    __asm__ __volatile__ ("movq %%rax,%%cr0" :: "a"((cr0)))
/* void set_cr3(void *) */
#define set_cr3(cr3)    __asm__ __volatile__ ("movq %%rax,%%cr3" :: "a"((cr3)))
/* void set_cr4(u64) */
#define set_cr4(cr4)    __asm__ __volatile__ ("movq %%rax,%%cr4" :: "a"((cr4)))
/* void xsave(void *) */
#define xsave(mem, a, d)                                                \
    __asm__ __volatile__ ("xsave64 (%%rdi)" :: "D"(mem), "a"(a), "d"(d));
#define xgetbv(a, b)                                                    \
    __asm__ __volatile__ ("xgetbv; movq %%rax,%%dr1" : "=a"(a), "=d"(b));

#define interrupt_handler_begin(handler)        \
    void handler(void) {                        \
        __asm__ __volatile__ ("pushq %rax;"     \
                              "pushq %rbx;"     \
                              "pushq %rcx;"     \
                              "pushq %rdx;"     \
                              "pushq %r8;"      \
                              "pushq %r9;"      \
                              "pushq %r10;"     \
                              "pushq %r11;"     \
                              "pushq %r12;"     \
                              "pushq %r13;"     \
                              "pushq %r14;"     \
                              "pushq %r15;"     \
                              "pushq %rsi;"     \
                              "pushq %rdi;"     \
                              "pushq %rbp;"     \
                              "pushw %fs;"      \
                              "pushw %gs;"      \
            );
#define interrupt_handler_end                   \
    __asm__ __volatile__ ("popw %gs;"           \
                          "popw %fs;"           \
                          "popq %rbp;"          \
                          "popq %rdi;"          \
                          "popq %rsi;"          \
                          "popq %r15;"          \
                          "popq %r14;"          \
                          "popq %r13;"          \
                          "popq %r12;"          \
                          "popq %r11;"          \
                          "popq %r10;"          \
                          "popq %r9;"           \
                          "popq %r8;"           \
                          "popq %rdx;"          \
                          "popq %rcx;"          \
                          "popq %rbx;"          \
                          "popq %rax;"          \
                          "iretq;");            \
    }


#define APIC_LAPIC_ID 0x020
#define MSR_APIC_BASE 0x1b
#define str(a)  st(a)
#define st(a)   #a
#define interrupt_task_restart                                  \
    __asm__ __volatile__ ("movq $" str(MSR_APIC_BASE) ",%rcx;"  \
                          "rdmsr;"                              \
                          "shlq $32,%rdx;"                      \
                          "addq %rax,%rdx;"                     \
                          "andq $0xfffffffffffff000,%rdx;"      \
                          "xorq %rax,%rax;"                     \
                          "movl " str(APIC_LAPIC_ID) "(%rdx),%eax;" \
                          /* Calculate the processor data space from the APIC ID */ \
                          "movq $" str(CPU_DATA_SIZE) ",%rbx;"          \
                          "mulq %rbx; /* [%rdx:%rax] = %rax * %rbx */"  \
                          "addq $" str(CPU_DATA_BASE) ",%rax;"          \
                          "movq %rax,%rbp;"                             \
                          /* If the next task is not scheduled, immediately restart this task */ \
                          "cmpq $0," str(CPU_NEXT_TASK_OFFSET) "(%rbp);" \
                          "jz 2f;");



#endif /* _KERNEL_ARCH_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
