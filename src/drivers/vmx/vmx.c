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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <machine/sysarch.h>
#include "vmx.h"

#define CPUID1H_ECX_VMX         5
#define CR4_VME                 (1ULL << 0)
#define CR4_MCE                 (1ULL << 6)

void *vmxon_vmcs;

int
vmx_enable(void)
{
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t vmx;
    uint64_t fixled0;
    uint64_t fixled1;
    struct sysarch_msr msr;
    uint64_t reg;
    uint32_t *vmcs;

    rax = 1;
    __asm__ __volatile__ ("cpuid" : "=c"(rcx), "=d"(rdx) : "a"(rax));

    /* Check VMX support */
    if ( !(rcx & (1 << CPUID1H_ECX_VMX)) ) {
        /* VMX is not supported by this CPU. */
        return -1;
    }

    /* Allocate VMCS data structure */
    vmcs = malloc(4096);
    if ( NULL == vmcs ) {
        return -1;
    }
    kmemset(vmcs, 0, 4096);

    /* Read VMX basic information
     * [30:0]  : Revision
     * [44:32] : Size of VMXON/VMCS region
     * [48]    : The width of the physical addresses in VMX
     * [49]    : Support of dual-monitor treatment of system-management
     *           interrupts and system-management mode
     * [53:50] : Memory type; 0: Uncacheable, 6: Write Back
     * [54]    : Reporting information in the VM-exit instruction-information
     *           field on VM exits due to execution of the INS and OUTS
     *           instructions
     * [55]    : VMX control (TRUE / default)
     */
    msr.key = IA32_VMX_BASIC;
    sysarch(SYSARCH_RDMSR, &msr);
    vmx = msr.value;

    /* Set up CR0 */
    msr.key = IA32_VMX_CR0_FIXED0;
    sysarch(SYSARCH_RDMSR, &msr);
    fixed0 = msr.value;
    msr.key = IA32_VMX_CR0_FIXED1;
    sysarch(SYSARCH_RDMSR, &msr);
    fixed1 = msr.value;
    sysarch(SYSARCH_GETCR0, &reg);
    sysarch(SYSARCH_SETCR0, (void *)((reg | fixed0) & fixed1));

    /* Set up CR4 */
    msr.key = IA32_VMX_CR4_FIXED0;
    sysarch(SYSARCH_RDMSR, &msr);
    fixed0 = msr.value;
    msr.key = IA32_VMX_CR4_FIXED1;
    sysarch(SYSARCH_RDMSR, &msr);
    fixed1 = msr.value;
    sysarch(SYSARCH_GETCR4, &reg);
    reg = ((reg | fixed0) & fixed1) | CR4_VME | CR4_MCE;
    sysarch(SYSARCH_SETCR4, (void *)reg);

    msr.key = IA32_FEATURE_CONTROL;
    sysarch(SYSARCH_RDMSR, &msr);
    reg = msr.value;
    /* 0: Lock, 1: Enable VMX in SMX operation, 2: Enable VMX outside SMX
       operation */
    if ( !(reg & 1) ) {
        /* Not locked, then enable VMX */
        msr.key = IA32_FEATURE_CONTROL;
        msr.value = reg | 5;
        sysarch(SYSARCH_WRMSR, &msr);
    }

    /* VMX on */
    vmcs[0] = vmx & 0x7fffffff;
    //phyaddr = addr_v2p(vmcs);
    //ret = vmxon(&phyaddr);
    //if ( ret ) {
    //    free(vmcs)
    //    return -1;
    //}
    vmxon_vmcs = vmcs;

    return 0;
}

/*
 * Entry point for the process manager program
 */
int
main(int argc, char *argv[])
{
    int ret;

    /* Enable VMX */
    ret = vmx_enable();
    if ( ret < 0 ) {
        exit(EXIT_FAILURE);
    }

    while ( 1 ) {
        vmx_enable();
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
