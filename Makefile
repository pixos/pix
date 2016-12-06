#
# Copyright (c) 2015-2016 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

VERSION = v0.0.1

all:
	@echo "make all is not currently supported."

## Compile initramfs (including kernel as well)
initrd:
#	Compile programs
	VERSION=${VERSION} make -C src init
	VERSION=${VERSION} make -C src pm
	VERSION=${VERSION} make -C src tty
	VERSION=${VERSION} make -C src pash
	VERSION=${VERSION} make -C src pci
	VERSION=${VERSION} make -C src fe

#       Create an image
	@./create_initrd.sh init:/servers/init pm:/servers/pm \
		tty:/drivers/tty pash:/bin/pash pci:/drivers/pci \
		fe:/ids/fe

## Compile boot loader
bootloader:
#	Compile the initial program loader in MBR
	make -C src diskboot
#	Compile the PXE boot loader
	make -C src pxeboot
#	Compile the boot monitor called from diskboot
	make -C src bootmon

## Compile kernel
kernel:
	make -C src kpack

## Create FAT12/16 image
image: bootloader kernel initrd
#	Create the boot image
	@./create_image.sh src/diskboot src/bootmon src/kpack initramfs

## VMDK
vmdk: image
	cp pix.img pix.raw.img
	@printf '\000' | dd of=pix.raw.img bs=1 seek=268435455 conv=notrunc > /dev/null 2>&1
	qemu-img convert -f raw -O vmdk pix.img pix.vmdk
	rm -f pix.raw.img

## Test
test:
	make -C src/tests test-all

## Clean
clean:
	make -C src clean
	rm -f initramfs
	rm -f pix.img
