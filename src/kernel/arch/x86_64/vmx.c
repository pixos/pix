/*_
 * Copyright (c) 2015 Hirochika Asai <asai@jar.jp>
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
#include "memory.h"
#include "vmx.h"

#define IA32_VMX_BASIC 0x480
#define IA32_VMX_CR0_FIXED0 0x486
#define IA32_VMX_CR0_FIXED1 0x487
#define IA32_VMX_CR4_FIXED0 0x488
#define IA32_VMX_CR4_FIXED1 0x489
#define IA32_FEATURE_CONTROL 0x03a

#define IA32_VMX_PINBASED_CTLS 0x481
#define IA32_VMX_PROCBASED_CTLS 0x482
#define IA32_VMX_EXIT_CTLS 0x483
#define IA32_VMX_ENTRY_CTLS 0x484
#define IA32_VMX_MISC 0x485

#define IA32_VMX_VMCS_ENUM 0x48a
#define IA32_VMX_PROCBASED_CTLS2 0x48b
#define IA32_VMX_EPT_VPID_CAP 0x48c
#define IA32_VMX_TRUE_PINBASED_CTLS 0x48d
#define IA32_VMX_TRUE_PROCBASED_CTLS 0x48e
#define IA32_VMX_TRUE_EXIT_CTLS 0x48f
#define IA32_VMX_TRUE_ENTRY_CTLS 0x490
#define IA32_PAT 0x277
#define IA32_SMBASE 0x9e
#define IA32_DEBUGCTL 0x1d9
#define IA32_EFER 0x0c0000080

struct vmx_stackframe {
    u64 rbp;
    u64 rsi;
    u64 rdi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;
} __attribute__ ((packed));

extern struct kmem *g_kmem;

#if 0
const char *vm_exit_reasons[] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "VM-entry failure due to invalid guest state.",
    "",
};
#endif

#if 0
/* Code example */
{
    /* VMXON: See Vol. 3C, 31.5 */
    if ( vmx_enable() ) {
        panic("Failed on vmxon");
    }

    if ( vmx_initialize_vmcs() ) {
        panic("Failed on initialization");
    }

    if ( vmlaunch() ) {
        panic("Failed on vmlaunch");
    }
}
#endif

void *vmxon_vmcs;

/*
 * Enable VMX
 */
int
vmx_enable(void)
{
    u64 rcx;
    u64 rdx;
    u64 vmx;
    u64 cr;
    u64 fixed0;
    u64 fixed1;
    u32 *vmcs;
    int ret;
    void *phyaddr;
    u64 f;

    /* Check the VMX support */
    cpuid(1, &rcx, &rdx);
    if ( !(rcx & (1 << 5)) ) {
        /* VMX is not supported */
        return -1;
    }

    /* Read VMX basic information
     * [30:0]  : Revision
     * [44:32] : Size of VMXON/VMCS region
     * [48]    : The width of the physical addresses in VMX
     * [49]    : Support of dual-monitor treatment of system-management
     *           interrupts and system-management mode
     * [53:50] : Memory type; 0: Uncacheable, 6: Write Back
     */
    vmx = rdmsr(IA32_VMX_BASIC);

    /* Set up CR0 */
    fixed0 = rdmsr(IA32_VMX_CR0_FIXED0);
    fixed1 = rdmsr(IA32_VMX_CR0_FIXED1);
    cr = get_cr0();
    set_cr0((cr | fixed0) & fixed1);

    /* Setup CR4 */
    fixed0 = rdmsr(IA32_VMX_CR4_FIXED0);
    fixed1 = rdmsr(IA32_VMX_CR4_FIXED1);
    cr = get_cr4();
    set_cr4((cr | fixed0) & fixed1);
    //set_cr4(get_cr4() | (1 << 13));
    /* todo: MCE on */
    set_cr4(get_cr4() | (1 << 7));

    vmcs = kmalloc(4096);
    kmemset(vmcs, 0, 4096);
    vmcs[0] = vmx & 0x7fffffff;

    f = rdmsr(IA32_FEATURE_CONTROL);
    // 0: Lock, 1: Enable VMX in SMX operation, 2: Enable VMX outside SMX operation
    if ( !(f & 1) ) {
        /* Not locked, then enable VMX */
        wrmsr(IA32_FEATURE_CONTROL, f | 5);
    }
    phyaddr = arch_vmem_addr_v2p(g_kmem->space, vmcs);
    ret = vmxon(&phyaddr);
    if ( ret ) {
        return -1;
    }
    vmxon_vmcs = vmcs;

    return 0;
}

void vmx_vm_exit_handler(void);
__asm__ ("_vmx_vm_exit_handler:"
         "push %rax;"
         "push %rbx;"
         "push %rcx;"
         "push %rdx;"
         "push %rdi;"
         "movq %dr0,%rax;push %rax;"
         "movq %dr1,%rax;push %rax;"
         "movq %dr2,%rax;push %rax;"
         "movq %dr3,%rax;push %rax;"
         "movq %dr4,%rax;push %rax;"
         "movq %dr5,%rax;push %rax;"
         "movq %dr6,%rax;push %rax;"
         "movq %rsp,%rdi;"
         "call _vmx_vm_exit_handler_c;"
         "pop %rdi;"
         "pop %rdx;"
         "pop %rcx;"
         "pop %rbx;"
         "pop %rax;"
         "vmresume");

void
vmx_vm_exit_handler_c(u64 *stack)
{
    u64 rd;

    /* VM exit reason: See Table C-1. Basic Exit Reasons */
    rd = vmread(0x4402);

    rd = rd & 0xff;
    if ( 1 == rd ) {
        /* External interrupt */
        //panic("ExtIntr");
        sti();
        vmresume();
    } else if ( 12 == rd ) {
        /* Hlt */
        panic("Hlt");
        sti();
        halt();
        vmresume();
#if 0
    } else if ( 52 == rd ) {
        /* VMX-preemption timer expired */
        //panic("VM exit (preemption expired)");
        vmwrite(0x482e, 1500);
        vmresume();
#endif
    } else {
        char e[2048];
#if 0
        ksnprintf(e, 2048, "VM exit %d / %llx %llx %llx %llx, %llx %llx %x %x",
                  rd, vmread(0x4402),
                  vmread(0x4016), vmread(0x6400), vmread(0x6800),
                  vmx_control_vm_entry_controls,
                  rdmsr(IA32_VMX_TRUE_ENTRY_CTLS),
                  get_cr0(), get_cr4());
#else
        ksnprintf(e, 2048,
                  "VM Exit (Exit reason=%d) BASIC=%llx\r\n"
                  "  VM-exec %llx VM-exec2 %llx VM-entry %llx VM-exit %llx\r\n"
                  "  EPT=%llx (high=%llx)\r\n"
                  "  cr0 %.8lx cr3 %.8lx cr4 %.8lx\r\n"
                  "  cs=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  es=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  ss=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  ds=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  fs=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  gs=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  ldtr=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  tr=%lx base=%.8lx limit=%.8lx acc=%.8lx\r\n"
                  "  gdtr base=%.8lx limit=%.8lx\r\n"
                  "  idtr base=%.8lx limit=%.8lx\r\n"
                  "  Interruptibility=%llx, Activity state=%llx, smbase=%llx\r\n"
                  "  VMX-preemption timer value=%llx\r\n"
                  "  Sysenter cs=%lx esp=%llx eip=%llx\r\n"
                  "  dr7=%.8lx, rsp=%.8lx, rip=%.8lx, rflags=%.8lx\r\n"
                  "  pending debug exception=%llx, intr=%llx\r\n"
                  "  CR0 guest/host mask=%llx, CR4 xx=%llx\r\n"
                  "  CR0 read shadow=%llx, CR4 xx=%llx\r\n"
                  "  EFER=%llx, PAT=%llx %llx %llx EFER=%llx VPID=%llx %llx %llx GPADDR=%llx %llx\r\n"
                  "  dr6=%.8lx dr5=%.8lx dr4=%.8lx dr3=%.8lx dr2=%.8lx dr1=%.8lx dr0=%.8lx %.8lx %.8lx %.8lx %.8lx %.8lx\r\n",
                  rd, rdmsr(IA32_VMX_BASIC),
                  vmread(0x4002), vmread(0x401e), vmread(0x4012), vmread(0x400c),
                  vmread(0x201a), vmread(0x201b),
                  vmread(0x6800), vmread(0x6802), vmread(0x6804),
                  vmread(0x802), vmread(0x6808), vmread(0x4802), vmread(0x4816),
                  vmread(0x800), vmread(0x6806), vmread(0x4800), vmread(0x4814),
                  vmread(0x804), vmread(0x680a), vmread(0x4804), vmread(0x4818),
                  vmread(0x806), vmread(0x680c), vmread(0x4806), vmread(0x481a),
                  vmread(0x808), vmread(0x680e), vmread(0x4808), vmread(0x481c),
                  vmread(0x80a), vmread(0x6810), vmread(0x480a), vmread(0x481e),
                  vmread(0x80c), vmread(0x6812), vmread(0x480c), vmread(0x4820),
                  vmread(0x80e), vmread(0x6814), vmread(0x480e), vmread(0x4822),
                  vmread(0x6816), vmread(0x4810),
                  vmread(0x6818), vmread(0x4812),
                  vmread(0x4824), vmread(0x4826), vmread(0x4828),
                  vmread(0x482e),
                  vmread(0x482a), vmread(0x6824), vmread(0x6826),
                  vmread(0x681a), vmread(0x681c), vmread(0x681e), vmread(0x6820),
                  vmread(0x6822), vmread(0x4016),
                  vmread(0x6000), vmread(0x6002),
                  vmread(0x6004), vmread(0x6006),
                  vmread(0x2806), vmread(0x2804),
                  rdmsr(IA32_VMX_MISC), vmread(0x4018), rdmsr(IA32_EFER),
                  rdmsr(IA32_VMX_EPT_VPID_CAP), get_cr0(), get_cr4(), vmread(0x2400), vmread(0x200c),
                  stack[0], stack[1], stack[2], stack[3], stack[4], stack[5] , stack[6],
                  stack[7], stack[8], stack[9], stack[10], stack[11]
            );
#endif

        //rdmsr(IA32_VMX_MISC);
        // [4:0] : The VMX-preemption timer (if it is active) counts down by
        // 1 every time bit X (this value) in the TSC changes due to a
        // TSC increment.
        // [5] : VM exits store the value of IA32_EFER.LMA into the “IA-32e
        // mode guest” VM-entry control
        // [8:6] : supported activity; 6=>hlt, 7=>shutdown, 8=>wait-for-SIPI
        // [15] : RDMSR can be used in SMM to read IA32_SMBASE (9EH)
        // [24:16] : # of CR3-target values supported by this processor
        // [27:25] : 512 * (N + 1) is the recommended maximum number of MSRs
        // to be included in each list (the VM-exit MSR-store list, the VM-exit
        // MSR-load list, or the VM-entry MSR-load list)
        // [28] : bit 2 of IA32_SMM_MONITOR_CTL can be set
        // [29] : Software can use VMWRITE to write to any supported field
        // in the VMCS
        // [63:32] : 32-bit MSEG revision identifier used by the processor

        panic(e);
    }
}
/*
  • If the “load IA32_PAT” VM-entry control is 1, the value of
    the field for the IA32_PAT MSR must be one that
    could be written by WRMSR without fault at CPL 0.
    Specifically, each of the 8 bytes in the field must have one
    of the values 0 (UC), 1 (WC), 4 (WT), 5 (WP), 6 (WB), or 7 (UC-).
*/
/*
 * Create a new VMCS
 */
int
vmx_initialize_vmcs(void)
{
    long i;
    int ret;

    u32 *vmcs;
    u64 vmx;

    u8 *mem;
    u64 *ept;
    void *phyaddr;

    struct {
        u16 limit;
        u64 base;
    } __attribute__ ((packed)) desc;

    /* New VMCS */
    vmcs = kmalloc(4096);
    if ( NULL == vmcs ) {
        return -1;
    }
    kmemset(vmcs, 0, 4096);

    vmx = rdmsr(IA32_VMX_BASIC);
    vmcs[0] = vmx & 0x7fffffff;
    phyaddr = arch_vmem_addr_v2p(g_kmem->space, vmcs);
    /* Clear */
    if ( vmclear(&phyaddr) ) {
        kfree(vmcs);
        return -1;
    }
    /* Bring the VMCS active and current */
    if ( vmptrld(&phyaddr) ) {
        kfree(vmcs);
        return -1;
    }
    char e[512];
    ksnprintf(e, 512, "Failed on vmlaunch: %d %llx", vmread(0x4400), get_cr4());
    //panic(e);


    /* Allocate Code */
    mem = kmalloc(1024 * 1024 * 256);
    if ( NULL == mem ) {
        return -1;
    }
    /* Program */
    kmemset(mem, 0, 1024 * 1024 * 256);
    //kmemcpy(mem, NULL, 1024 * 1024 * 2);
    //kmemset(mem + 0xfe898, 0, 0x48);
    //mem[0x7c00] = 0xf4; // hlt
    // 0f 20 e0
    mem[0x7c00] = 0x0f; // mov %cr4, %rax
    mem[0x7c01] = 0x20;
    mem[0x7c02] = 0xe0;
    mem[0x7c03] = 0x90; // nop
    mem[0x7c04] = 0xeb; // jmp
    mem[0x7c05] = 0xfd; // back
    ept = kmalloc(4096 * 4);
    if ( NULL == ept ) {
        kfree(mem);
        return -1;
    }
    kmemset(ept, 0, 4096 * 4);
    ept[0] = 0x07 | (u64)arch_vmem_addr_v2p(g_kmem->space, &ept[512]);
    ept[512] = 0x07 | (u64)arch_vmem_addr_v2p(g_kmem->space, &ept[1024]);
    for ( i = 0; i < 128; i++ ) {
        phyaddr = arch_vmem_addr_v2p(g_kmem->space, mem);
        ept[1024 + i] = 0xb7 | ((u64)phyaddr + i * 1024 * 1024 * 2);
    }

    /* EPTP */
    phyaddr = arch_vmem_addr_v2p(g_kmem->space, ept);
    vmx_control_ept_pointer_full = 0x1e | (u64)phyaddr;

#if 0
    //char e[512];
    ksnprintf(e, 512, "%x %x %llx %llx %x %x %llx %llx %llx %llx %llx %llx %llx",
              vmx >> 55, get_cr3(),
              vmx_vm_exit_handler,
              arch_vmem_addr_v2p(g_kmem->space, vmx_vm_exit_handler),
              rdmsr(IA32_VMX_ENTRY_CTLS),
              rdmsr(IA32_VMX_TRUE_ENTRY_CTLS), rdmsr(IA32_VMX_MISC),
              rdmsr(IA32_DEBUGCTL), rdmsr(IA32_EFER),
              ept, arch_vmem_addr_v2p(g_kmem->space, ept),
              mem, arch_vmem_addr_v2p(g_kmem->space, mem));
    panic(e);
#endif

    /* Pin-based VM-execution controls
       0: External-interrupt exiting
       3: NMI exiting
       5: Virtual NMIs
       6: Activate VMX-preemption timer
       7: Process posted interrupts
     */
    //vmx_control_pin_based = 0x0000001f;
    if ( (vmx >> 55) & 1 ) {
        vmx_control_pin_based
            = ((rdmsr(IA32_VMX_TRUE_PINBASED_CTLS) & 0xffffff) | 0x00000049)
            & (rdmsr(IA32_VMX_TRUE_PINBASED_CTLS) >> 32);
    } else {
        vmx_control_pin_based
            = ((rdmsr(IA32_VMX_PINBASED_CTLS)) | 0x00000049)
            & (rdmsr(IA32_VMX_PINBASED_CTLS) >> 32);
    }
    /* Processor-Based VM-Execution Controls
       2: Interrupt-window exiting
       3: Use TSC offsetting
       7: HLT exiting
       9: INVLPG exiting
       10: MWAIT exiting
       11: PDPMC exiting
       12: RDTSC exiting
       15: CR3-load exiting
       16: CR3-store exiting
       19: CR8-load exiting
       20: CR8-store exiting
       21: Use TPR shadown
       22: NMI-window exiting
       23: MOV-DR exiting
       24: Unconditional I/O exiting
       25: Use I/O bitmaps
       27: Monitor trap flag
       28: Use MSR bitmaps
       29: MONITOR exiting
       30: PAUSE exiting
       31: Activate secondary controls
    */
    //vmx_control_primary_processor_based = 0x8401e9f2;
    if ( (vmx >> 55) & 1 ) {
        vmx_control_primary_processor_based
            = ((rdmsr(IA32_VMX_TRUE_PROCBASED_CTLS) & 0xffffffff) | 0x80018880)
            & (rdmsr(IA32_VMX_TRUE_PROCBASED_CTLS) >> 32);
    } else {
        vmx_control_primary_processor_based
            = ((rdmsr(IA32_VMX_PROCBASED_CTLS) & 0xffffffff) | 0x80018880)
            & (rdmsr(IA32_VMX_PROCBASED_CTLS) >> 32);
    }
    /* VM-Exit Controls
       2: Save debug controls
       9: Host address-space size
       12: Load IA32_PERF_GLOBAL_CTRL
       15: Acknowledge interrupt on exit
       18: Save IA32_PAT
       19: Load IA32_PAT
       20: Save IA32_EFER
       21: Load IA32_EFER
       22: Save VMX-preemption timer value
    */
    //vmx_control_vm_exit_controls = 0x00036fff;
    if ( (vmx >> 55) & 1 ) {
        vmx_control_vm_exit_controls
            = ((rdmsr(IA32_VMX_TRUE_EXIT_CTLS) & 0xffffffff) | 0x007c0204)
            & (rdmsr(IA32_VMX_TRUE_EXIT_CTLS) >> 32);
    } else {
        vmx_control_vm_exit_controls
            = ((rdmsr(IA32_VMX_EXIT_CTLS) & 0xffffffff) | 0x0007c0204)
            & (rdmsr(IA32_VMX_EXIT_CTLS) >> 32);
    }
    /* VM-Entry Controls
       2: Load debug control
       9: IA-32e mode guest
       10: Entry to SMM
       11: Deactivate dual-monitor treatment
       13: Load IA32_PERF_GLOBAL_CTRL
       14: Load IA32_PAT
       15: Load IA32_EFER
    */
    //vmx_control_vm_entry_controls = 0x000011ff;
    if ( (vmx >> 55) & 1 ) {
        vmx_control_vm_entry_controls
            = ((rdmsr(IA32_VMX_TRUE_ENTRY_CTLS) & 0xffffffff) | 0xc004)
            & (rdmsr(IA32_VMX_TRUE_ENTRY_CTLS) >> 32);
    } else {
        vmx_control_vm_entry_controls
            = ((rdmsr(IA32_VMX_ENTRY_CTLS) & 0xffffffff) | 0xc004)
            & (rdmsr(IA32_VMX_ENTRY_CTLS) >> 32);
    }
    /* Secondary Processor-Based VM-Execution Controls
       0: Virtualized APIC access
       1: Enable EPT
       2: Descriptor-table exiting
       3: Enable RDTSCP
       4: Virtualize x2APIC mode
       5: Enable VPID
       6: WBINVD exiting
       7: Unrestricted guest
       8: APIC-register virtualization
       9: Virtual-interrupt delivery
       10: PAUSE-loop exiting
       11: RDRAND exiting
       12: Enable INVPCID
       13: Enable VM functions
       14: VMCS shadowing
       18: EPT-violation #VE
    */
    //vmx_control_secondary_processor_based
    //    = rdmsr(IA32_VMX_PROCBASED_CTLS2) | 0x00000082;
    vmx_control_secondary_processor_based
        = ((rdmsr(IA32_VMX_PROCBASED_CTLS2) & 0xffffffff) | 0x00000082)
        & (rdmsr(IA32_VMX_PROCBASED_CTLS2) >> 32);

    //vmx_control_cr3_target_count = (rdmsr(IA32_VMX_MISC) >> 16) & 0x1ff;
    vmx_control_cr3_target_count = 0;
    vmx_control_io_bitmap_a_full = 0;
    vmx_control_io_bitmap_b_full = 0;
    vmx_control_msr_bitmaps_full = 0;
    vmx_control_tsc_offset_full = 0;
    vmx_control_vm_entry_msr_load_full = 0;

    __asm__ __volatile__ ( "movq %%es,%%rax" : "=a"(vmx_host_es_selector) );
    __asm__ __volatile__ ( "movq %%cs,%%rax" : "=a"(vmx_host_cs_selector) );
    __asm__ __volatile__ ( "movq %%ss,%%rax" : "=a"(vmx_host_ss_selector) );
    __asm__ __volatile__ ( "movq %%ds,%%rax" : "=a"(vmx_host_ds_selector) );
    __asm__ __volatile__ ( "movq %%fs,%%rax" : "=a"(vmx_host_fs_selector) );
    __asm__ __volatile__ ( "movq %%gs,%%rax" : "=a"(vmx_host_gs_selector) );
    __asm__ __volatile__ ( "xorq %%rax,%%rax; str %%ax"
                           : "=a"(vmx_host_tr_selector) );
    vmx_host_efer_full = rdmsr(IA32_EFER); /* EFER MSR */
    vmx_host_pat_full = rdmsr(IA32_PAT); /* PAT MSR */
    vmx_host_cr0 = get_cr0();
    vmx_host_cr3 = (u64)get_cr3();
    vmx_host_cr4 = get_cr4();
    sgdt(&desc);
    //vmx_host_gdtr_base = 0x75048;
    //vmx_host_gdtr_base = 0x74000;//desc.base;
    vmx_host_gdtr_base = desc.base;
    sidt(&desc);
    //vmx_host_idtr_base = 0x77000;
    //vmx_host_idtr_base = 0x76000;//desc.base;
    vmx_host_idtr_base = desc.base;
    //vmx_host_rsp = (u64)arch_vmem_addr_v2p(g_kmem->space, kmalloc(4096));
    //vmx_host_rsp = 0x9000;
    //vmx_host_rsp = 0x01f00000;
    vmx_host_rsp = (u64)kmalloc(4096);
    //kmemset(vmx_host_rsp, 0, 4096 * 4);
    if ( 0 == vmx_host_rsp ) {
        return -1;
    }
    vmx_host_rsp += 4096;
    vmx_host_rip = (u64)vmx_vm_exit_handler;

    vmx_host_sysenter_cs = 0;
    vmx_host_fs_base = 0;
    vmx_host_gs_base = 0;
    vmx_host_tr_base = 0;
    vmx_host_sysenter_esp = 0;
    vmx_host_sysenter_eip = 0;

    vmx_guest_es_selector = 0x0;
    vmx_guest_cs_selector = 0x0;
    vmx_guest_ss_selector = 0x0;
    vmx_guest_ds_selector = 0x0;
    vmx_guest_fs_selector = 0x0;
    vmx_guest_gs_selector = 0x0;
    vmx_guest_ldtr_selector = 0x0;
    vmx_guest_tr_selector = 0x0;
    vmx_guest_es_limit = 0x0000ffff;
    vmx_guest_cs_limit = 0x0000ffff;
    vmx_guest_ss_limit = 0x0000ffff;
    vmx_guest_ds_limit = 0x0000ffff;
    vmx_guest_fs_limit = 0x0000ffff;
    vmx_guest_gs_limit = 0x0000ffff;
    //vmx_guest_tr_limit = 0x0000ffff;
    vmx_guest_tr_limit = 0x000000ff;
    //vmx_guest_ldtr_limit = 0xffffffff;
    vmx_guest_ldtr_limit = 0x0000ffff;
    //vmx_guest_gdtr_limit = 0x0000ffff;
    vmx_guest_gdtr_limit = 0x00000047;
    vmx_guest_idtr_limit = 0x0000ffff;
    vmx_guest_es_access_rights = 0x00000093;
    vmx_guest_cs_access_rights = 0x0000009b;
    //vmx_guest_cs_access_rights = 0x0000009f;
    vmx_guest_ss_access_rights = 0x00000093;
    vmx_guest_ds_access_rights = 0x00000093;
    vmx_guest_fs_access_rights = 0x00000093;
    vmx_guest_gs_access_rights = 0x00000093;
    //vmx_guest_ldtr_access_rights = 0x00010000;
    vmx_guest_ldtr_access_rights = 0x00000082;
    vmx_guest_tr_access_rights = 0x0000008b;
    //vmx_guest_tr_access_rights = 0x00000083;

    vmx_guest_cr0 = 0x00000030;
    //vmx_guest_cr0 = 0x00000010;
    vmx_guest_cr3 = 0;
    //vmx_guest_cr3 = 0x79000;
    vmx_guest_cr4 = 1 << 13;
    vmx_guest_es_base = 0;
    vmx_guest_cs_base = 0;
    vmx_guest_ss_base = 0;
    vmx_guest_ds_base = 0;
    vmx_guest_fs_base = 0;
    vmx_guest_gs_base = 0;
    vmx_guest_ldtr_base = 0;
    vmx_guest_tr_base = 0;
    vmx_guest_gdtr_base = 0xfe898;//0;
    vmx_guest_idtr_base = 0;
    vmx_guest_dr7 = 0x00000400;
    vmx_guest_rsp = 0x1000;
    vmx_guest_rip = 0x7c00;
    vmx_guest_rflags = 0x0202;
    //vmx_guest_rflags = 0x0002;
    vmx_guest_pending_debug_exceptions = 0x00000000;
    vmx_guest_sysenter_esp = 0x00000000;
    vmx_guest_sysenter_eip = 0x00000000;
    vmx_guest_sysenter_cs = 0;
    //vmx_preemption_timer_value = 1500;
    vmx_preemption_timer_value = 5000;

    /* Activity state; 0: active, 1: HLT, 2: Shutdown, 3: Wait-for-SIPI */
    vmx_guest_activity_state = 0;

    //vmx_control_cr0_mask = 0x80000021;
    vmx_control_cr0_mask = 0;
    //vmx_control_cr0_read_shadow = 0x80000021;
    vmx_control_cr0_read_shadow = 0;
    vmx_control_cr4_mask = 0x00002000;
    //vmx_control_cr4_mask = 0;
    //vmx_control_cr4_read_shadow = 0x00002000;
    vmx_control_cr4_read_shadow = 0;

    vmx_guest_efer_full = 0;
    vmx_guest_interruptibility_state = 0;

    vmx_guest_vmcs_link_pointer_full = 0xffffffffffffffffULL;

    vmx_guest_pat_full = rdmsr(IA32_PAT);

    vmx_guest_debugctl_full = 0;
    //vmx_guest_smbase = 0;

#if 0
    vmx_control_executive_vmcs_pointer_full
        = arch_vmem_addr_v2p(g_kmem->space, vmxon_vmcs);
    vmx_control_executive_vmcs_pointer_full = 0;
#endif

    for ( i = 0; i < sizeof(vmx_vmcs) / sizeof(struct vmx_vmcs); i++ ) {
        ret = vmwrite(vmx_vmcs[i].index, *(vmx_vmcs[i].ptr));
        if ( ret ) {
            panic("xxx");
            return -1;
        }
    }
    //wrmsr(0x0c0000080, 0);
    //ksnprintf(e, 512, "Failed on vmlaunch: %d %llx", vmread(0x4400), get_cr4());
    //panic(e);

#if 0
    phyaddr = arch_vmem_addr_v2p(g_kmem->space, vmcs);
    /* Clear */
    if ( vmclear(&phyaddr) ) {
        return -1;
    }
    /* Bring the VMCS active and current */
    if ( vmptrld(&phyaddr) ) {
        return -1;
    }
#endif

    return 0;
}

/*
 * Get the error code of the previous instruction
 */
u64
vmx_get_error(void)
{
    return vmread(0x4400);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
