#
# Copyright (c) 2013 Scyphus Solutions Co. Ltd.
# Copyright (c) 2014-2015 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

all:
	@echo "make all is not currently supported."

pix.img:
	make -C src diskboot
	cp src/diskboot pix.img
	printf '\125\252' | dd of=./pix.img bs=1 seek=510 conv=notrunc

image: pix.img
