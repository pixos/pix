#
# Copyright (c) 2015-2016 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

ARCH=x86_64

## Global flags
ASFLAGS=-nostdlib -I./kernel/arch/$(ARCH)/
CFLAGS=-I./include -Wall -fleading-underscore -nostdinc -nostdlib -O3 -m64 \
	-DARCH_X86_64=1 -DPIX_VERSION='"$(VERSION)"'

## Override the flags
diskboot: ASFLAGS=-nostdlib -I./boot/arch/$(ARCH)/
bootmon: ASFLAGS=-nostdlib -I./boot/arch/$(ARCH)/
bootmon: CFLAGS=-I./include \
	-Wall -fleading-underscore -nostdlib -nodefaultlibs -fno-builtin -O3 -m64

pxeboot: ASFLAGS=-nostdlib -I./boot/arch/$(ARCH)/

kpack: ASFLAGS=-nostdlib -I./kernel/arch/$(ARCH)/
kpack: CFLAGS=-I./include \
	-Wall -fleading-underscore -mcmodel=large -nostdlib -nodefaultlibs \
	-fno-builtin -O3 -m64 -DARCH_X86_64=1

## IPL
diskboot: boot/arch/$(ARCH)/diskboot.o
	$(LD) -N -e start -Ttext=0x7c00 --oformat binary -o $@ $^

## Boot monitor
bootmon: boot/arch/$(ARCH)/bootmon.o \
	boot/arch/$(ARCH)/kernload.o \
	boot/arch/$(ARCH)/entry16.o \
	boot/arch/$(ARCH)/entry32.o \
	boot/arch/$(ARCH)/entry64.o \
	boot/arch/$(ARCH)/boot.o
	$(LD) -N -e bootmon -Ttext=0x9000 --oformat binary -o $@ $^

## PXE boot
pxeboot: boot/arch/$(ARCH)/pxeboot.o
	$(LD) -N -e pxeboot -Ttext=0x7c00 --oformat binary -o $@ $^

## Kernel
kernel/arch/$(ARCH)/arch.o: include/aos/const.h kernel/kernel.h \
	kernel/arch/$(ARCH)/arch.h \
	include/sys/syscall.h
kernel/arch/$(ARCH)/desc.o: include/aos/const.h kernel/kernel.h \
	kernel/arch/$(ARCH)/arch.h \
	kernel/arch/$(ARCH)/desc.h
kernel/arch/$(ARCH)/acpi.o: include/aos/const.h kernel/kernel.h \
	kernel/arch/$(ARCH)/arch.h \
	kernel/arch/$(ARCH)/acpi.h
kernel/arch/$(ARCH)/i8254.o: include/aos/const.h kernel/kernel.h \
	kernel/arch/$(ARCH)/arch.h \
	kernel/arch/$(ARCH)/i8254.h
kernel/kernel.o: include/aos/const.h kernel/kernel.h
kernel/ramfs.o: include/aos/const.h kernel/kernel.h
kernel/syscall.o: include/aos/const.h kernel/kernel.h
kpack: kernel/arch/$(ARCH)/asm.o \
	kernel/arch/$(ARCH)/arch.o \
	kernel/arch/$(ARCH)/desc.o \
	kernel/arch/$(ARCH)/acpi.o \
	kernel/arch/$(ARCH)/apic.o \
	kernel/arch/$(ARCH)/i8254.o \
	kernel/arch/$(ARCH)/memory.o \
	kernel/arch/$(ARCH)/trampoline.o \
	kernel/arch/$(ARCH)/ap_entry32.o \
	kernel/arch/$(ARCH)/ap_entry64.o \
	kernel/arch/$(ARCH)/vmx.o \
	kernel/arch/$(ARCH)/task.o \
	kernel/kernel.o \
	kernel/memory.o \
	kernel/pmem.o \
	kernel/kmem.o \
	kernel/vmem.o \
	kernel/strfmt.o \
	kernel/sched.o \
	kernel/rbtree.o \
	kernel/mpq.o \
	kernel/ramfs.o \
	kernel/devfs.o \
	kernel/syscall.o \
	kernel/syspix.o \
	kernel/sysdriver.o
	$(LD) -N -e kstart64 -Ttext=0xc0010000 --oformat binary -o $@ $^
	$(LD) -N -e kstart64 -Ttext=0xc0010000 -o kpack.dbg $^

## libc
LIBCOBJS=lib/arch/$(ARCH)/libc.o lib/arch/$(ARCH)/libcasm.o lib/stdio/fio.o \
	lib/stdio/print.o lib/string/str.o
lib/arch/$(ARCH)/libc.o: lib/crt0.o

## init server
init: servers/init/init.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## process manager
pm: servers/pm/pm.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## forwarding engine
fe: ids/fe/fe.o ids/fe/pci.o $(LIBCOBJS) lib/driver.o
	$(LD) -T app.ld -o $@ $^
	$(LD) -T appdebug.ld -o $@.dbg $^

## file system
fs: servers/fs/fs.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## TCP server
tcp: servers/tcp/tcp_main.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## tty driver (including keyboard, video, and serial drivers)
tty: drivers/tty/tty.o drivers/tty/console.o drivers/tty/kbd.o \
	drivers/tty/serial.o drivers/tty/linebuf.o $(LIBCOBJS) lib/driver.o
	$(LD) -T app.ld -o $@ $^

## shell
pash: bin/pash/pash.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## PCI driver
pci: drivers/pci/pci.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## VMX driver
vmx: drivers/vmx/vmx.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## Clean
clean:
	find . -name "*.o" | xargs rm -f
	rm -f init
	rm -f pm
	rm -f kpack
	rm -f bootmon
	rm -f diskboot
