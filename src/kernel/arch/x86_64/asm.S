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

#include "const.h"

	.text

	.code64
	.globl	kstart64
	.globl	apstart64
	.globl	_halt
	.globl	_pause
	.globl	_lgdt
	.globl	_sgdt
	.globl	_lidt
	.globl	_sidt
	.globl	_lldt
	.globl	_ltr
	.globl	_sti
	.globl	_cli
	.globl	_rdtsc
	.globl	_inb
	.globl	_inw
	.globl	_inl
	.globl	_outb
	.globl	_outw
	.globl	_outl
	.globl	_mfread32
	.globl	_mfwrite32
	.globl	_cpuid
	.globl	_rdmsr
	.globl	_wrmsr
	.globl	_kmemset
	.globl	_kmemcmp
	.globl	_kmemcpy
	.globl	_bitwidth
	.globl	_spin_lock_intr
	.globl	_spin_unlock_intr
	.globl	_spin_lock
	.globl	_spin_unlock
	.globl	_syscall_setup
	.globl	_asm_ioapic_map_intr
	.globl	_get_cr0
	.globl	_get_cr3
	.globl	_get_cr4
	.globl	_invlpg
	.globl	_vmxon
	.globl	_vmclear
	.globl	_vmptrld
	.globl	_vmwrite
	.globl	_vmread
	.globl	_vmlaunch
	.globl	_vmresume
	.globl	_task_restart
	.globl	_task_replace
	.globl	_intr_null
	.globl	_intr_dze
	.globl	_intr_debug
	.globl	_intr_nmi
	.globl	_intr_breakpoint
	.globl	_intr_overflow
	.globl	_intr_bre
	.globl	_intr_iof
	.globl	_intr_dna
	.globl	_intr_df
	.globl	_intr_itf
	.globl	_intr_snpf
	.globl	_intr_ssf
	.globl	_intr_gpf
	.globl	_intr_pf
	.globl	_intr_x87_fpe
	.globl	_intr_simd_fpe
	.globl	_intr_acf
	.globl	_intr_mca
	.globl	_intr_vef
	.globl	_intr_se
	.globl	_intr_apic_loc_tmr
	.globl	_intr_apic_loc_tmr_xp
	.globl	_intr_crash
	.globl	_sys_fork
	.globl	_sys_task_switch
	.globl	_vmx_vm_exit_handler
	.globl	_vmx_vm_exit_handler_resume

	.set	APIC_LAPIC_ID,0x020
	.set	APIC_EOI,0x0b0
	.set	MSR_APIC_BASE,0x1b

/* Entry point to the 64-bit kernel */
kstart64:
	/* Disable interrupts */
	cli

	/* Re-configure the stack pointer (for alignment) */
	/* Obtain APIC ID */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	xorq	%rax,%rax
	movl	APIC_LAPIC_ID(%rdx),%eax
	shrl	$24,%eax
	/* P6 family and Pentium processors: [27:24] */
	/* Pentium 4 processors, Xeon processors, and later processors: [31:24] */

	/* Setup stack with 16 byte guard */
	addl	$1,%eax
	movl	$CPU_DATA_SIZE,%ebx
	mull	%ebx			/* [%edx|%eax] = %eax * %ebx */
	movabs	$CPU_DATA_BASE,%rdx
	addq	%rdx,%rax
	subq	$CPU_STACK_GUARD,%rax
	movq	%rax,%rsp

	/* Initialize the bootstrap processor */
	call	_bsp_init
	/* Start the kernel code */
	call	_kmain
	jmp	_halt

/* Entry point for the application processors */
apstart64:
	/* Disable interrupts */
	cli

	/* Re-configure the stack pointer (for alignment) */
	/* Obtain APIC ID */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	xorq	%rax,%rax
	movl	APIC_LAPIC_ID(%rdx),%eax
	shrl	$24,%eax
	/* P6 family and Pentium processors: [27:24] */
	/* Pentium 4 processors, Xeon processors, and later processors: [31:24] */

	/* Setup stack with 16 byte guard */
	addl	$1,%eax
	movl	$CPU_DATA_SIZE,%ebx
	mull	%ebx			/* [%edx|%eax] = %eax * %ebx */
	movabs	$CPU_DATA_BASE,%rdx
	addq	%rdx,%rax
	subq	$CPU_STACK_GUARD,%rax
	movq	%rax,%rsp

	/* Initialize the application processor */
	call	_ap_init
	/* Start the kernel code */
	call	_kmain
	jmp	_halt

/* void halt(void) */
_halt:
	sti
	hlt
	ret

/* void pause(void) */
_pause:
	pause
	ret

/* void lgdt(void *gdtr, u64 selector) */
_lgdt:
	lgdt	(%rdi)
	/* Reload GDT */
	pushq	%rsi
	movabs	$1f,%rax
	pushq	%rax	/* Just to do ret */
	lretq
1:
	/* Set data selector */
	movq	%rsi,%rax
	addq	$8,%rax
	movq	%rax,%ds
	movq	%rax,%es
	movq	%rax,%ss
	ret

/* void sgdt(void *gdtr) */
_sgdt:
	sgdt	(%rdi)
	ret

/* void lidt(void *idtr) */
_lidt:
	lidt	(%rdi)
	ret

/* void sidt(void *idtr) */
_sidt:
	sidt	(%rdi)
	ret

/* void lldt(u16) */
_lldt:
	lldt	%di
	ret

/* void ltr(u16) */
_ltr:
	ltr	%di
	ret

/* void sti(void) */
_sti:
	sti
	ret

/* void cli(void) */
_cli:
	cli
	ret

/* u64 rdtsc(void) */
_rdtsc:
	xorq	%rax,%rax
	movq	%rax,%rdx
	rdtscp
	shlq	$32,%rdx
	addq	%rdx,%rax
	ret

/* u8 inb(u16 port) */
_inb:
	movw	%di,%dx
	xorq	%rax,%rax
	inb	%dx,%al
	ret

/* u16 inw(u16 port) */
_inw:
	movw	%di,%dx
	xorq	%rax,%rax
	inw	%dx,%ax
	ret

/* u32 inw(u16 port) */
_inl:
	movw	%di,%dx
	xorq	%rax,%rax
	inl	%dx,%eax
	ret

/* void outb(u16 port, u8 value) */
_outb:
	movw	%di,%dx
	movw	%si,%ax
	outb	%al,%dx
	ret

/* void outw(u16 port, u16 value) */
_outw:
	movw	%di,%dx
	movw	%si,%ax
	outw	%ax,%dx
	ret

/* void outl(u16 port, u32 value) */
_outl:
	movw	%di,%dx
	movl	%esi,%eax
	outl	%eax,%dx
	ret

/* u32 mfread32(u64 addr) */
_mfread32:
	mfence			/* Prevent out-of-order execution */
	movl	(%rdi),%eax
	ret

/* void mfwrite32(u64 addr, u32 val) */
_mfwrite32:
	mfence			/* Prevent out-of-order execution */
	movl	%esi,(%rdi)
	ret

/* u64 cpuid(u64 rax, u64 *rcx, u64 *rdx) */
_cpuid:
	movq	%rdi,%rax
	movq	%rdx,%rdi
	cpuid
	movq	%rcx,(%rsi)
	movq	%rdx,(%rdi)
	ret

/* u64 rdmsr(u64 reg) */
_rdmsr:
	movq	%rdi,%rcx
	rdmsr
	shlq	$32,%rdx
	addq	%rdx,%rax
	ret

/* void wrmsr(u64 reg, u64 data) */
_wrmsr:
	movq	%rdi,%rcx
	movq	%rsi,%rax
	movq	%rax,%rdx
	shrq	$32,%rdx
	wrmsr
	ret

/* void * kmemset(void *b, int c, size_t len) */
_kmemset:
	pushq	%rdi
	pushq	%rsi
	movl	%esi,%eax	/* c */
	movq	%rdx,%rcx	/* len */
	cld			/* Ensure the DF cleared */
	rep	stosb		/* Set %al to (%rdi)-(%rdi+%rcx) */
	popq	%rsi
	popq	%rdi
	movq	%rdi,%rax	/* Restore for the return value */
	ret

/* int kmemcmp(void *s1, void *s2, size_t n) */
_kmemcmp:
	xorq	%rax,%rax
	movq	%rdx,%rcx	/* n */
	cld			/* Ensure the DF cleared */
	repe	cmpsb		/* Compare byte at (%rsi) with byte at (%rdi) */
	jz	1f
	decq	%rdi		/* rollback one */
	decq	%rsi		/* rollback one */
	movb	(%rdi),%al	/* *s1 */
	subb	(%rsi),%al	/* *s1 - *s2 */
1:
	ret

/* int kmemcpy(void *__restrict dst, void *__restrict src, size_t n) */
_kmemcpy:
	movq	%rdi,%rax	/* Return value */
	movq	%rdx,%rcx	/* n */
	cld			/* Ensure the DF cleared */
	rep	movsb		/* Copy byte at (%rsi) to (%rdi) */
	ret

/* u64 bitwidth(u64) */
_bitwidth:
	decq	%rdi
	xorq	%rax,%rax
	testq	%rdi,%rdi
	jz	1f
	bsrq	%rdi,%rax
	incq	%rax
1:
	ret

/* void spin_lock_intr(u32 *) */
_spin_lock_intr:
	cli
/* void spin_lock(u32 *) */
_spin_lock:
	xorl	%ecx,%ecx
	incl	%ecx
1:
	xorl	%eax,%eax
	lock cmpxchgl	%ecx,(%rdi)
	jnz	1b
	ret

/* void spin_unlock(u32 *) */
_spin_unlock:
	xorl	%eax,%eax
	lock xchgl	(%rdi),%eax
	ret

/* void spin_unlock_intr(u32 *) */
_spin_unlock_intr:
	xorl	%eax,%eax
	lock xchgl	(%rdi),%eax
	sti
	ret


/* void syscall_setup(void *, u64 nr) */
_syscall_setup:
	movabs	$SYSCALL_TABLE,%rax
	movq	%rdi,(%rax)
	movabs	$SYSCALL_NR,%rax
	movq	%rsi,(%rax)
	/* Write syscall flag mask */
	movq	$0xc0000084,%rcx	/* IA32_FMASK */
	movq	$0x3200,%rax
	movq	%rax,%rdx
	shrq	$32,%rdx
	wrmsr
	/* Write syscall entry point */
	movq	$0xc0000082,%rcx	/* IA32_LSTAR */
	movabs	$syscall_entry,%rax
	movq	%rax,%rdx
	shrq	$32,%rdx
	wrmsr
	/* Segment register */
	movq	$0xc0000081,%rcx
	movq	$0x0,%rax
	/* IA32_STAR: syscall; cs=[47:32], ss=[47:32]+8 */
	/* sysret; 32-bit cs=[63:48], 64-bit cs=[63:48]+16, ss=[63:48]+8 */
	movq	$(GDT_RING0_CODE_SEL | ((GDT_RING3_CODE32_SEL + 3) << 16)),%rdx
	wrmsr
	/* Enable syscall */
	movl	$0xc0000080,%ecx	/* EFER MSR number */
	rdmsr
	btsl	$0,%eax		/* SYSCALL ENABLE bit [bit 0] = 1 */
	wrmsr
	ret


/* Entry point to a syscall */
syscall_entry:
	/* rip and rflags are stored in rcx and r11, respectively. */
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rcx		/* -8(%rbp): rip */
	pushq	%r11		/* -16(%rbp): rflags */
	pushq	%rbx

	/* Check the number */
	movabs	$SYSCALL_NR,%rbx
	cmpq	(%rbx),%rax
	jge	1f

	/* Lookup the system call table and call one corresponding to %rax */
	movabs	$SYSCALL_TABLE,%rcx
	movq	(%rcx),%rbx
	cmpq	$0,%rbx
	je	1f
	shlq	$3,%rax		/* 8 byte for each function pointer */
	addq	%rax,%rbx
	movq	%r10,%rcx	/* Replace the 4th argument with %r10 */
	callq	*(%rbx)		/* Call the function */
1:
	popq	%rbx
	popq	%r11
	popq	%rcx
	popq	%rbp
	sysretq

/* pid_t sys_fork(void) */
_sys_fork:
	pushq	%rbp
	movq	%rsp,%rbp
	subq	$8,%rsp
	movq	%rsp,%rdi
	subq	$8,%rsp
	movq	%rsp,%rsi
	subq	$8,%rsp
	movq	%rsp,%rdx
	call	_sys_fork_c
	addq	$24,%rsp
	cmpl	$0,%eax
	jne	1f
	movq	-8(%rsp),%rdi
	movq	-16(%rsp),%rsi
	movq	-24(%rsp),%rdx
	call	sys_fork_restart
1:
	/* Return upon error */
	leaveq
	ret

/* void sys_fork_restart(u64 task, u64 ret0, u64, ret1) */
sys_fork_restart:
	movq	%rdx,%rax
	leaveq			/* Restore the stack */
	addq	$8,%rsp		/* Pop the return point */
	pushq	%rdi
	pushq	%rsi

	/* Setup the stackframe for the forked task */
	movq	TASK_RP(%rdi),%rdx
	addq	$164,%rdx
	movq	$GDT_RING0_DATA_SEL,%rcx
	movq	%rcx,-8(%rdx)	/* ss */
	movq	%rbp,%rcx
	movq	%rcx,-16(%rdx)	/* rsp */
	pushfq
	popq	%rcx
	movq	%rcx,-24(%rdx)	/* rflags */
	movq	$GDT_RING0_CODE_SEL,%rcx
	movq	%rcx,-32(%rdx)	/* cs */
	movabs	$1f,%rcx
	movq	%rcx,-40(%rdx)	/* rip */
	movq	%rsi,-48(%rdx)	/* rax */
	movq	-24(%rbp),%rcx
	movq	%rcx,-56(%rdx)	/* rbx */
	movq	-8(%rbp),%rcx
	movq	%rcx,-64(%rdx)	/* rcx */
	movq	-16(%rbp),%rcx
	movq	%rcx,-104(%rdx)	/* r11 */
	movq	%r12,-112(%rdx)	/* r12 */
	movq	%r13,-120(%rdx)	/* r13 */
	movq	%r14,-128(%rdx)	/* r14 */
	movq	%r15,-136(%rdx)	/* r15 */
	movq	-32(%rbp),%rcx
	movq	%rcx,-144(%rdx)	/* rsi */
	movq	-40(%rbp),%rcx
	movq	%rcx,-152(%rdx)	/* rdi */
	movq	0(%rbp),%rcx
	//movq	$0xabcd,%rcx
	movq	%rcx,-160(%rdx)	/* rbp */
	movw	$(GDT_RING3_DATA64_SEL+3),%cx
	movw	%cx,-162(%rdx)	/* fs */
	movw	%cx,-164(%rdx)	/* gs */

	/* Restore */
	popq	%rsi
	popq	%rdi
	popq	%rbx
	popq	%r11
	popq	%rcx
	movq	%rbp,%rsp
	popq	%rbp
	sysretq
1:
	popq	%rbp
	sysretq

/* int sys_task_switch(const struct timespec *rqtp, struct timespec *rmtp) */
_sys_task_switch:
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rbx
	pushq	%rcx
	pushq	%rdx
	pushq	%rdi
	pushq	%rsi
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	/* Get the APIC ID */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	xorq	%rax,%rax
	movl	APIC_LAPIC_ID(%rdx),%eax
	shrl	$24,%eax
	/* Calculate the processor data space from the APIC ID */
	movq	$CPU_DATA_SIZE,%rbx
	mulq	%rbx		/* [%rdx:%rax] = %rax * %rbx */
	movabs	$CPU_DATA_BASE,%rdx
	addq	%rdx,%rax
	movq	%rax,%rbx

	/* If the current task is null, then do nothing. */
	cmpq	$0,CPU_CUR_TASK_OFFSET(%rbx)
	jz	3f

	/* Save the stack frame for the currnt task */
	movq	%rsp,%rdx
	movabs	$3f,%rax		/* for rip to restart */
	pushq	$GDT_RING0_DATA_SEL	/* ss */
	pushq	%rdx			/* rsp */
	pushf				/* rflags */
	pushq	$GDT_RING0_CODE_SEL	/* cs */
	pushq	%rax			/* rip */
	pushq	$0			/* rax */
	pushq	%rbx
	pushq	%rcx
	pushq	$0			/* rdx */
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushq	%rsi
	pushq	%rdi
	pushq	%rbp
	pushw	%fs
	pushw	%gs

	/* If the next task is not scheduled, idle task should be scheduled. */
	cmpq	$0,CPU_NEXT_TASK_OFFSET(%rbx)
	jnz	1f
	movq	CPU_IDLE_TASK_OFFSET(%rbx),%rcx	/* idle task */
	jmp	2f
1:
	movq	CPU_NEXT_TASK_OFFSET(%rbx),%rcx /* next task */
2:
	/* Save the stack pointer (restart point) */
	movq	CPU_CUR_TASK_OFFSET(%rbx),%rax
	movq	%rsp,TASK_RP(%rax)
	/* Save the FPU/SSE registers */
	movq	TASK_XREGS(%rax),%rdi
	movq	$5,%rax
	movq	$0,%rdx
	clts
	xsave64	(%rdi)

	/* Notify that the current task is switched (to the kernel) */
	movq	CPU_CUR_TASK_OFFSET(%rbx),%rdi
	movq	%rcx,%rsi	/* next task */
	callq	_arch_task_switched
	/* Task switch (set the stack frame of the new task) */
	movq	%rcx,CPU_CUR_TASK_OFFSET(%rbx)
	movq	TASK_RP(%rcx),%rsp
	movq	$0,CPU_NEXT_TASK_OFFSET(%rbx)
	/* Change page table */
	movq	TASK_CR3(%rcx),%rdx
	movq	%rdx,%cr3
	/* Restore FPU/SSE registers */
	movq	TASK_XREGS(%rcx),%rdi
	movq	$5,%rax
	movq	$0,%rdx
	xrstor64	(%rdi)
	/* Setup sp0 in TSS */
	movq	CPU_CUR_TASK_OFFSET(%rbx),%rax
	movq	TASK_SP0(%rax),%rdx
	leaq	CPU_TSS_OFFSET(%rbx),%rax
	movq	%rdx,TSS_SP0(%rax)

	/* Pop all registers from the stackframe */
	popw	%gs
	popw	%fs
	popq	%rbp
	popq	%rdi
	popq	%rsi
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdx
	popq	%rcx
	popq	%rbx
	popq	%rax
	iretq
3:
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rsi
	popq	%rdi
	popq	%rdx
	popq	%rcx
	popq	%rbx
	leaveq
	ret


/* void asm_ioapic_map_intr(u64 val, u64 tbldst, u64 ioapic_base) */
_asm_ioapic_map_intr:
	/* Copy arguments */
	movq	%rdi,%rax       /* src */
	movq	%rsi,%rcx       /* tbldst */
	/* rdx = ioapic_base */

	/* *(u32 *)(ioapic_base + 0x00) = tbldst * 2 + 0x10 */
	shlq	$1,%rcx         /* tbldst * 2 */
	addq	$0x10,%rcx      /* tbldst * 2 + 0x10 */
	sfence
	movl	%ecx,0x00(%rdx) /* IOREGSEL (0x00) */
	/* *(u32 *)(ioapic_base + 0x10) = (u32)src */
	sfence
	movl	%eax,0x10(%rdx) /* IOWIN (0x10) */
	shrq	$32,%rax
	/* *(u32 *)(ioapic_base + 0x00) = tbldst * 2 + 0x10 + 1 */
	addq	$1,%rcx         /* tbldst * 2 + 0x10 + 1 */
	sfence
	movl	%ecx,0x00(%rdx)
	/* *(u32 *)(ioapic_base + 0x10) = (u32)(src >> 32) */
	sfence
	movl	%eax,0x10(%rdx)
	ret

/* u64 get_cr0(void) */
_get_cr0:
	movq	%cr0,%rax
	ret

/* void * get_cr3(void) */
_get_cr3:
	movq	%cr3,%rax
	ret

/* u64 get_cr4(void) */
_get_cr4:
	movq	%cr4,%rax
	ret

/* void invlpg(void *) */
_invlpg:
	invlpg	(%rdi)
	ret

/* int vmxon(void *) */
_vmxon:
	vmxon	(%rdi)
	jz	1f
	sbbq	%rax,%rax
	ret
1:
	movq	$-1,%rax
	ret

/* int vmclear(void *) */
_vmclear:
	vmclear	(%rdi)
	jz	1f
	sbbq	%rax,%rax
	ret
1:
	movq	$-1,%rax
	ret

/* int vmptrld(void *) */
_vmptrld:
	vmptrld	(%rdi)
	jz	1f
	sbbq	%rax,%rax
	ret
1:
	movq	$-1,%rax
	ret

/* int vmwrite(u64, u64) */
_vmwrite:
	vmwrite	%rsi,%rdi
	jz	1f
	sbbq	%rax,%rax
	ret
1:
	movq	$-1,%rax
	ret

/* u64 vmread(u64) */
_vmread:
	vmread	%rdi,%rax
	ret


/* int vmlaunch(void) */
_vmlaunch:
	vmlaunch
	jz	1f
	sbbq	%rax,%rax
	ret
1:
	movq	$-1,%rax
	ret

/* int vmresume(void) */
_vmresume:
	vmresume
	jz	1f
	sbbq	%rax,%rax
	ret
1:
	movq	$-1,%rax
	ret

/* Null interrupt handler (do nothing) */
_intr_null:
	iretq
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	/* APIC EOI */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr			/* Read APIC info to [%edx:%eax]; N.B., higer */
				/*  32 bits of %rax and %rdx are cleared */
				/*  bit [35:12]: APIC Base, [11]: EN */
				/*  [10]: EXTD, and [8]:BSP */
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	movl	$0,APIC_EOI(%rdx)	/* EOI */
	popq	%rdx
	popq	%rcx
	popq	%rax
	iretq

/* Macro for generic exception handlers */
.macro	intr_exception_generic name vec
	.globl	_intr_\name
_intr_\name:
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rax
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%rbx
	movq	$\vec,%rdi
	movq	8(%rbp),%rsi	/* rip */
	movq	16(%rbp),%rdx	/* cs */
	movq	24(%rbp),%rcx	/* rflags */
	movq	32(%rbp),%r8	/* rsp */
	/* 40(%rbp): ss */
	call	_isr_exception
	popq	%rbx
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rax
	popq	%rbp
	iretq
.endm

/* Macro for exception handlers that hold an error code */
.macro	intr_exception_werror name vec
	.globl	_intr_\name
_intr_\name:
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rax
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%rbx
	movq	$\vec,%rdi
	movq	8(%rbp),%rsi	/* error code */
	movq	16(%rbp),%rdx	/* rip */
	movq	24(%rbp),%rcx	/* cs */
	movq	32(%rbp),%r8	/* rflags */
	movq	40(%rbp),%r9	/* rsp */
	/* 48(%rbp): ss */
	call	_isr_exception_werror
	popq	%rbx
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rax
	popq	%rbp
	addq	$0x8,%rsp
	iretq
.endm

/* Divide-by-zero Error (#DE) */
	intr_exception_generic dze 0x00
/* Debug fault or trap */
	intr_exception_generic debug 0x01
/* Non-maskable Interrupt */
	intr_exception_generic nmi 0x02
/* Breakpoint (#BP) */
	intr_exception_generic breakpoint 0x03
/* Overflow (#OF) */
	intr_exception_generic overflow 0x04
/* Bound Range Exceeded (#BR) */
	intr_exception_generic bre 0x05
/* Invalid Opcode (#UD) */
	intr_exception_generic iof 0x06
/* Device Not Available (#NM) */
_intr_dna:
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rax
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%rbx
	movq	8(%rbp),%rdx	/* rip */
	movq	16(%rbp),%rcx	/* cs */
	movq	24(%rbp),%r8	/* rflags */
	movq	32(%rbp),%r9	/* rsp */
	/* 48(%rbp): ss */
	call	_isr_exception
	popq	%rbx
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rax
	popq	%rbp
	iretq
/* Double Fault (#DF) */
	intr_exception_werror df 0x08
/* Coprocessor Segment Overrun: To be ignored */
	//intr_exception_generic cso 0x09
.globl	_intr_cso
_intr_cso:
	iretq
/* Invalid TSS (#TS) */
	intr_exception_werror invtss 0x0a
/* Segment Not Present (#NP) */
	intr_exception_werror snpf 0x0b
/* Stack-Segment Fault (#SS) */
	intr_exception_werror ssf 0x0c
/* General Protection Fault (#GP) */
	intr_exception_werror gpf 0x0d
/* Page Fault (#PF) */
_intr_pf:
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rax
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%rbx
	movq	%cr2,%rdi	/* virtual address */
	movq	8(%rbp),%rsi	/* error code */
	movq	16(%rbp),%rdx	/* rip */
	movq	24(%rbp),%rcx	/* cs */
	movq	32(%rbp),%r8	/* rflags */
	movq	40(%rbp),%r9	/* rsp */
	/* 48(%rbp): ss */
	call	_isr_page_fault
	popq	%rbx
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rax
	popq	%rbp
	addq	$0x8,%rsp
	iretq
/* x87 Floating-Point Exception (#MF) */
	intr_exception_generic x87_fpe 0x10
/* Alignment Check (#AC) */
	intr_exception_werror acf 0x11
/* Machine Check (#MC) */
	intr_exception_generic mca 0x12
/* SIMD Floating-Point Exception (#XM/#XF) */
	intr_exception_generic simd_fpe 0x13
/* Virtualization Exception (#VE) */
	intr_exception_generic vef 0x14
/* Security Exception (#SX) */
	intr_exception_werror se 0x1e


/* macro to save registers to the stackframe and call the interrupt handler */
.macro	intr_lapic_isr vec
	pushq	%rax
	pushq	%rbx
	pushq	%rcx
	pushq	%rdx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushq	%rsi
	pushq	%rdi
	pushq	%rbp
	pushw	%fs
	pushw	%gs
	/* Set CR0.TS */
	movq	%cr0,%rax
	bts	$3,%rax
	movq	%rax,%cr0
	/* Call kernel function for ISR */
	movq	$\vec,%rdi
	callq	_kintr_isr
	clts
	/* EOI for the local APIC */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr			/* Read APIC info to [%edx:%eax]; N.B., higer */
				/*  32 bits of %rax and %rdx are cleared */
				/*  bit [35:12]: APIC Base, [11]: EN */
				/*  [10]: EXTD, and [8]:BSP */
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	movl	$0,APIC_EOI(%rdx)	/* EOI */
.endm

/* macro to restore from the stackframe */
.macro	intr_lapic_isr_done
	/* Pop all registers from the stackframe */
	popw	%gs
	popw	%fs
	popq	%rbp
	popq	%rdi
	popq	%rsi
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdx
	popq	%rcx
	popq	%rbx
	popq	%rax
.endm

.macro	intr_lapic_isr_generic vec
	.globl	_intr_driver_\vec
_intr_driver_\vec:
	intr_lapic_isr	\vec
	jmp	_task_restart
.endm

/* Interrupt handler for local APIC timer */
_intr_apic_loc_tmr:
	intr_lapic_isr 0x40
	jmp	_task_restart

/* Null interrupt handler (do nothing) */
_intr_apic_loc_tmr_xp:
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	/* APIC EOI */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr			/* Read APIC info to [%edx:%eax]; N.B., higer */
				/*  32 bits of %rax and %rdx are cleared */
				/*  bit [35:12]: APIC Base, [11]: EN */
				/*  [10]: EXTD, and [8]:BSP */
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	movl	$0,APIC_EOI(%rdx)	/* EOI */
	popq	%rdx
	popq	%rcx
	popq	%rax
	iretq

/* IRQn */
	intr_lapic_isr_generic 0x20
	intr_lapic_isr_generic 0x21
	intr_lapic_isr_generic 0x22
	intr_lapic_isr_generic 0x23
	intr_lapic_isr_generic 0x24
	intr_lapic_isr_generic 0x25
	intr_lapic_isr_generic 0x26
	intr_lapic_isr_generic 0x27
	intr_lapic_isr_generic 0x28
	intr_lapic_isr_generic 0x29
	intr_lapic_isr_generic 0x2a
	intr_lapic_isr_generic 0x2b
	intr_lapic_isr_generic 0x2c
	intr_lapic_isr_generic 0x2d
	intr_lapic_isr_generic 0x2e
	intr_lapic_isr_generic 0x2f
/* For driver use */
	intr_lapic_isr_generic 0x50
	intr_lapic_isr_generic 0x51
	intr_lapic_isr_generic 0x52
	intr_lapic_isr_generic 0x53
	intr_lapic_isr_generic 0x54
	intr_lapic_isr_generic 0x55
	intr_lapic_isr_generic 0x56
	intr_lapic_isr_generic 0x57
	intr_lapic_isr_generic 0x58
	intr_lapic_isr_generic 0x59
	intr_lapic_isr_generic 0x5a
	intr_lapic_isr_generic 0x5b
	intr_lapic_isr_generic 0x5c
	intr_lapic_isr_generic 0x5d
	intr_lapic_isr_generic 0x5e
	intr_lapic_isr_generic 0x5f
	intr_lapic_isr_generic 0x60
	intr_lapic_isr_generic 0x61
	intr_lapic_isr_generic 0x62
	intr_lapic_isr_generic 0x63
	intr_lapic_isr_generic 0x64
	intr_lapic_isr_generic 0x65
	intr_lapic_isr_generic 0x66
	intr_lapic_isr_generic 0x67
	intr_lapic_isr_generic 0x68
	intr_lapic_isr_generic 0x69
	intr_lapic_isr_generic 0x6a
	intr_lapic_isr_generic 0x6b
	intr_lapic_isr_generic 0x6c
	intr_lapic_isr_generic 0x6d
	intr_lapic_isr_generic 0x6e
	intr_lapic_isr_generic 0x6f
	intr_lapic_isr_generic 0x70
	intr_lapic_isr_generic 0x71
	intr_lapic_isr_generic 0x72
	intr_lapic_isr_generic 0x73
	intr_lapic_isr_generic 0x74
	intr_lapic_isr_generic 0x75
	intr_lapic_isr_generic 0x76
	intr_lapic_isr_generic 0x77
	intr_lapic_isr_generic 0x78
	intr_lapic_isr_generic 0x79
	intr_lapic_isr_generic 0x7a
	intr_lapic_isr_generic 0x7b
	intr_lapic_isr_generic 0x7c
	intr_lapic_isr_generic 0x7d
	intr_lapic_isr_generic 0x7e
	intr_lapic_isr_generic 0x7f
	intr_lapic_isr_generic 0x80
	intr_lapic_isr_generic 0x81
	intr_lapic_isr_generic 0x82
	intr_lapic_isr_generic 0x83
	intr_lapic_isr_generic 0x84
	intr_lapic_isr_generic 0x85
	intr_lapic_isr_generic 0x86
	intr_lapic_isr_generic 0x87
	intr_lapic_isr_generic 0x88
	intr_lapic_isr_generic 0x89
	intr_lapic_isr_generic 0x8a
	intr_lapic_isr_generic 0x8b
	intr_lapic_isr_generic 0x8c
	intr_lapic_isr_generic 0x8d
	intr_lapic_isr_generic 0x8e
	intr_lapic_isr_generic 0x8f
	intr_lapic_isr_generic 0x90
	intr_lapic_isr_generic 0x91
	intr_lapic_isr_generic 0x92
	intr_lapic_isr_generic 0x93
	intr_lapic_isr_generic 0x94
	intr_lapic_isr_generic 0x95
	intr_lapic_isr_generic 0x96
	intr_lapic_isr_generic 0x97
	intr_lapic_isr_generic 0x98
	intr_lapic_isr_generic 0x99
	intr_lapic_isr_generic 0x9a
	intr_lapic_isr_generic 0x9b
	intr_lapic_isr_generic 0x9c
	intr_lapic_isr_generic 0x9d
	intr_lapic_isr_generic 0x9e
	intr_lapic_isr_generic 0x9f
	intr_lapic_isr_generic 0xa0
	intr_lapic_isr_generic 0xa1
	intr_lapic_isr_generic 0xa2
	intr_lapic_isr_generic 0xa3
	intr_lapic_isr_generic 0xa4
	intr_lapic_isr_generic 0xa5
	intr_lapic_isr_generic 0xa6
	intr_lapic_isr_generic 0xa7
	intr_lapic_isr_generic 0xa8
	intr_lapic_isr_generic 0xa9
	intr_lapic_isr_generic 0xaa
	intr_lapic_isr_generic 0xab
	intr_lapic_isr_generic 0xac
	intr_lapic_isr_generic 0xad
	intr_lapic_isr_generic 0xae
	intr_lapic_isr_generic 0xaf
	intr_lapic_isr_generic 0xb0
	intr_lapic_isr_generic 0xb1
	intr_lapic_isr_generic 0xb2
	intr_lapic_isr_generic 0xb3
	intr_lapic_isr_generic 0xb4
	intr_lapic_isr_generic 0xb5
	intr_lapic_isr_generic 0xb6
	intr_lapic_isr_generic 0xb7
	intr_lapic_isr_generic 0xb8
	intr_lapic_isr_generic 0xb9
	intr_lapic_isr_generic 0xba
	intr_lapic_isr_generic 0xbb
	intr_lapic_isr_generic 0xbc
	intr_lapic_isr_generic 0xbd
	intr_lapic_isr_generic 0xbe
	intr_lapic_isr_generic 0xbf
	intr_lapic_isr_generic 0xc0
	intr_lapic_isr_generic 0xc1
	intr_lapic_isr_generic 0xc2
	intr_lapic_isr_generic 0xc3
	intr_lapic_isr_generic 0xc4
	intr_lapic_isr_generic 0xc5
	intr_lapic_isr_generic 0xc6
	intr_lapic_isr_generic 0xc7
	intr_lapic_isr_generic 0xc8
	intr_lapic_isr_generic 0xc9
	intr_lapic_isr_generic 0xca
	intr_lapic_isr_generic 0xcb
	intr_lapic_isr_generic 0xcc
	intr_lapic_isr_generic 0xcd
	intr_lapic_isr_generic 0xce
	intr_lapic_isr_generic 0xcf
	intr_lapic_isr_generic 0xd0
	intr_lapic_isr_generic 0xd1
	intr_lapic_isr_generic 0xd2
	intr_lapic_isr_generic 0xd3
	intr_lapic_isr_generic 0xd4
	intr_lapic_isr_generic 0xd5
	intr_lapic_isr_generic 0xd6
	intr_lapic_isr_generic 0xd7
	intr_lapic_isr_generic 0xd8
	intr_lapic_isr_generic 0xd9
	intr_lapic_isr_generic 0xda
	intr_lapic_isr_generic 0xdb
	intr_lapic_isr_generic 0xdc
	intr_lapic_isr_generic 0xdd
	intr_lapic_isr_generic 0xde
	intr_lapic_isr_generic 0xdf

/* Crash interrupt */
_intr_crash:
	cli
1:
	hlt
	jmp	1b


/* Task restart */
_task_restart:
	/* Get the APIC ID */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	xorq	%rax,%rax
	movl	APIC_LAPIC_ID(%rdx),%eax
	shrl	$24,%eax
	/* Calculate the processor data space from the APIC ID */
	movq	$CPU_DATA_SIZE,%rbx
	mulq	%rbx		/* [%rdx:%rax] = %rax * %rbx */
	movabs	$CPU_DATA_BASE,%rdx
	addq	%rdx,%rax
	movq	%rax,%rbp
	/* If the next task is not scheduled, immediately restart this task. */
	cmpq	$0,CPU_NEXT_TASK_OFFSET(%rbp)
	jz	2f
	movq	CPU_NEXT_TASK_OFFSET(%rbp),%rax
	/* If the current task is null, then do not need to save anything. */
	cmpq	$0,CPU_CUR_TASK_OFFSET(%rbp)
	jz	1f
	/* Save the stack pointer (restart point) */
	movq	CPU_CUR_TASK_OFFSET(%rbp),%rax
	movq	%rsp,TASK_RP(%rax)
	/* Save the FPU/SSE registers */
	movq	TASK_XREGS(%rax),%rdi
	movq	$5,%rax
	movq	$0,%rdx
	clts
	xsave64	(%rdi)
1:
	/* Notify that the current task is switched (to the kernel) */
	movq	CPU_CUR_TASK_OFFSET(%rbp),%rdi
	movq	CPU_NEXT_TASK_OFFSET(%rbp),%rsi
	callq	_arch_task_switched
	/* Task switch (set the stack frame of the new task) */
	movq	CPU_NEXT_TASK_OFFSET(%rbp),%rax
	movq	%rax,CPU_CUR_TASK_OFFSET(%rbp)
	movq	TASK_RP(%rax),%rsp
	movq	$0,CPU_NEXT_TASK_OFFSET(%rbp)
	/* Change page table */
	movq	TASK_CR3(%rax),%rdx
	movq	%rdx,%cr3
	/* Restore FPU/SSE registers */
	movq	TASK_XREGS(%rax),%rdi
	movq	$5,%rax
	movq	$0,%rdx
	xrstor64	(%rdi)
	/* Setup sp0 in TSS */
	movq	CPU_CUR_TASK_OFFSET(%rbp),%rax
	movq	TASK_SP0(%rax),%rdx
	leaq	CPU_TSS_OFFSET(%rbp),%rax
	movq	%rdx,TSS_SP0(%rax)
2:
	intr_lapic_isr_done
	iretq

/* Replace the current task with the task pointed  by %rdi */
_task_replace:
	movq	TASK_RP(%rdi),%rsp
	/* Change page table */
	movq	TASK_CR3(%rdi),%rax
	movq	%rax,%cr3
	/* Restore FPU/SSE registers */
	movq	TASK_XREGS(%rdi),%rbx
	movq	$5,%rax
	movq	$0,%rdx
	xrstor64	(%rbx)
	/* Get the APIC ID */
	movq	$MSR_APIC_BASE,%rcx
	rdmsr
	shlq	$32,%rdx
	addq	%rax,%rdx
	andq	$0xfffffffffffff000,%rdx	/* APIC Base */
	xorq	%rax,%rax
	movl	APIC_LAPIC_ID(%rdx),%eax
	shrl	$24,%eax
	/* Calculate the processor data space from the APIC ID */
	movq	$CPU_DATA_SIZE,%rbx
	mulq	%rbx		/* [%rdx:%rax] = %rax * %rbx */
	movabs	$CPU_DATA_BASE,%rdx
	addq	%rdx,%rax
	movq	%rax,%rbp
	/* Setup sp0 in TSS */
	movq	TASK_SP0(%rdi),%rdx
	leaq	CPU_TSS_OFFSET(%rbp),%rax
	movq	%rdx,TSS_SP0(%rax)
	intr_lapic_isr_done
	iretq


/* VM Exit handler */
_vmx_vm_exit_handler:
	push	%rax
	push	%rbx
	push	%rcx
	push	%rdx
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rdi
	push	%rsi
	push	%rbp
	movq	%dr0, %rax
	push	%rax
	movq	%dr1, %rax
	push	%rax
	movq	%dr2, %rax
	push	%rax
	movq	%dr3, %rax
	push	%rax
	movq	%dr6, %rax
	push	%rax
	movq	%rsp, %rdi
	call	_vmx_vm_exit_handler_c

/* Resume procedure from the VM Exit handler */
_vmx_vm_exit_handler_resume:
	pop	%rax
	movq	%rax, %dr6
	pop	%rax
	movq	%rax, %dr3
	pop	%rax
	movq	%rax, %dr2
	pop	%rax
	movq	%rax, %dr1
	pop	%rax
	movq	%rax,%dr0
	pop	%rbp
	pop	%rsi
	pop	%rdi
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdx
	pop	%rcx
	pop	%rbx
	pop	%rax
	vmresume
	jz 1f
	sbbq	%rax, %rax
	ret
1:
	movq	$-1, %rax
	ret
