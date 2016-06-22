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

extern struct kmem *g_kmem;

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
    //set_cr4(cr | (1 << 13));

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

    return 0;
}


void
vmx_vm_exit_handler(void)
{
    u64 rd;

    /* VM exit reason: See Table C-1. Basic Exit Reasons */
    rd = vmread(0x4402);

    rd = rd & 0xff;
    if ( 1 == rd ) {
        /* External interrupt */
        sti();
        vmresume();
    } else if ( 12 == rd ) {
        /* Hlt */
        sti();
        halt();
        vmresume();
    } else if ( 52 == rd ) {
        /* VMX-preemption timer expired */
        panic("VM exit (preemption expired)");
    } else {
        char e[512];
        ksnprintf(e, 512, "VM exit %d", rd);
        panic(e);
    }
}

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
    //mem[0x7c00] = 0xf4; // hlt
    mem[0x7c00] = 0x90; // nop
    mem[0x7c01] = 0xeb; // jmp
    mem[0x7c02] = 0xfd; // back
    ept = kmalloc(4096 * 3);
    if ( NULL == ept ) {
        kfree(mem);
        return -1;
    }
    kmemset(ept, 0, 4096 * 3);
    ept[0] = 0x07 | (u64)arch_vmem_addr_v2p(g_kmem->space, &ept[512]);
    ept[512] = 0x07 | (u64)arch_vmem_addr_v2p(g_kmem->space, &ept[1024]);
    for ( i = 0; i < 128; i++ ) {
        phyaddr = arch_vmem_addr_v2p(g_kmem->space, mem);
        ept[1024 + i] = 0x87 | ((u64)phyaddr + i * 1024 * 1024 * 2);
    }

    /* EPTP */
    phyaddr = arch_vmem_addr_v2p(g_kmem->space, ept);
    vmx_control_ept_pointer_full = 0x58 | (u64)phyaddr;
    vmx_control_ept_pointer_high = 0;

#if 0
    char e[512];
    ksnprintf(e, 512, "%x %x %llx %llx %x %x",
              vmx >> 55, get_cr3(),
              vmx_vm_exit_handler,
              arch_vmem_addr_v2p(g_kmem->space, vmx_vm_exit_handler),
              rdmsr(IA32_VMX_ENTRY_CTLS),
              rdmsr(IA32_VMX_TRUE_ENTRY_CTLS));
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
        vmx_control_pin_based = 0x00000009 | rdmsr(IA32_VMX_TRUE_PINBASED_CTLS);
    } else {
        vmx_control_pin_based = 0x00000009 | rdmsr(IA32_VMX_PINBASED_CTLS);
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
            = rdmsr(IA32_VMX_TRUE_PROCBASED_CTLS) | 0x80018880;
    } else {
        vmx_control_primary_processor_based
            = rdmsr(IA32_VMX_PROCBASED_CTLS) | 0x80018880;
    }
    /* VM-Exit Controls
       2: Save debug controls
       9: Host address-space size
       12: Load IA32_PREF_GLOBAL_CTRL
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
            = rdmsr(IA32_VMX_TRUE_EXIT_CTLS) | 0x00030204;
    } else {
        vmx_control_vm_exit_controls = rdmsr(IA32_VMX_EXIT_CTLS) | 0x00030204;
    }
    /* VM-Entry Controls
       2: Load debug control
       9: IA-32e mode guest
       10: Entry to SMM
       11: Deactivate dual-monitor treatment
       13: Load IA32_PREF_GLOBAL_CTRL
       14: Load IA32_PAT
       15: Load IA32_EFER
    */
    //vmx_control_vm_entry_controls = 0x000011ff;
    if ( (vmx >> 55) & 1 ) {
        vmx_control_vm_entry_controls = rdmsr(IA32_VMX_TRUE_ENTRY_CTLS) | 0x4;
    } else {
        vmx_control_vm_entry_controls = rdmsr(IA32_VMX_ENTRY_CTLS) | 0x4;
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
        = rdmsr(IA32_VMX_PROCBASED_CTLS2) | 0x00000082;

    //vmx_control_cr3_target_count = (rdmsr(IA32_VMX_MISC) >> 16) & 0x1ff;
    vmx_control_cr3_target_count = 0;
    vmx_control_io_bitmap_a_full = 0;
    vmx_control_io_bitmap_a_high = 0;
    vmx_control_io_bitmap_b_full = 0;
    vmx_control_io_bitmap_b_high = 0;
    vmx_control_msr_bitmaps_full = 0;
    vmx_control_msr_bitmaps_high = 0;
    vmx_control_tsc_offset_full = 0;
    vmx_control_tsc_offset_high = 0;

    __asm__ __volatile__ ( "movq %%es,%%rax" : "=a"(vmx_host_es_selector) );
    __asm__ __volatile__ ( "movq %%cs,%%rax" : "=a"(vmx_host_cs_selector) );
    __asm__ __volatile__ ( "movq %%ss,%%rax" : "=a"(vmx_host_ss_selector) );
    __asm__ __volatile__ ( "movq %%ds,%%rax" : "=a"(vmx_host_ds_selector) );
    __asm__ __volatile__ ( "movq %%fs,%%rax" : "=a"(vmx_host_fs_selector) );
    __asm__ __volatile__ ( "movq %%gs,%%rax" : "=a"(vmx_host_gs_selector) );
    __asm__ __volatile__ ( "xorq %%rax,%%rax; str %%ax"
                           : "=a"(vmx_host_tr_selector) );
    vmx_host_efer_full = rdmsr(0x0c0000080); /* EFER MSR */
    vmx_host_cr0 = get_cr0();
    vmx_host_cr3 = (u64)get_cr3();
    vmx_host_cr4 = get_cr4();
    sgdt(&desc);
    //vmx_host_gdtr_base = 0x75048;
    vmx_host_gdtr_base = 0x74000;//desc.base;
    sidt(&desc);
    //vmx_host_idtr_base = 0x77000;
    vmx_host_idtr_base = 0x76000;//desc.base;
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
    vmx_guest_es_limit = 0x0000ffff;
    vmx_guest_cs_limit = 0x0000ffff;
    vmx_guest_ss_limit = 0x0000ffff;
    vmx_guest_ds_limit = 0x0000ffff;
    vmx_guest_fs_limit = 0x0000ffff;
    vmx_guest_gs_limit = 0x0000ffff;
    vmx_guest_tr_limit = 0x000000ff;
    vmx_guest_ldtr_limit = 0xffffffff;
    vmx_guest_gdtr_limit = 0x0000ffff;
    vmx_guest_idtr_limit = 0x0000ffff;
    vmx_guest_es_access_rights = 0x00000093;
    vmx_guest_cs_access_rights = 0x0000009b;
    vmx_guest_ss_access_rights = 0x00000093;
    vmx_guest_ds_access_rights = 0x00000093;
    vmx_guest_fs_access_rights = 0x00000093;
    vmx_guest_gs_access_rights = 0x00000093;
    vmx_guest_ldtr_access_rights = 0x00010000;
    vmx_guest_tr_access_rights = 0x0000008b;

    vmx_guest_cr0 = 0x60000030;
    vmx_guest_cr3 = 0;
    vmx_guest_cr4 = 0;//1 << 13;
    vmx_guest_es_base = 0;
    vmx_guest_cs_base = 0;
    vmx_guest_ss_base = 0;
    vmx_guest_ds_base = 0;
    vmx_guest_fs_base = 0;
    vmx_guest_gs_base = 0;
    vmx_guest_ldtr_base = 0;
    vmx_guest_tr_base = 0;
    vmx_guest_gdtr_base = 0;
    vmx_guest_idtr_base = 0;
    vmx_guest_dr7 = 0x00000400;
    vmx_guest_rsp = 0;
    vmx_guest_rip = 0x7c00;
    vmx_guest_rflags = 2;
    vmx_guest_pending_debug_exceptions = 0x00000000;
    vmx_guest_sysenter_esp = 0x00000000;
    vmx_guest_sysenter_eip = 0x00000000;

    vmx_guest_vmcs_link_pointer_full = 0xffffffffffffffffULL;

    for ( i = 0; i < sizeof(vmx_vmcs) / sizeof(struct vmx_vmcs); i++ ) {
        ret = vmwrite(vmx_vmcs[i].index, *(vmx_vmcs[i].ptr));
        if ( ret ) {
            return -1;
        }
    }

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
