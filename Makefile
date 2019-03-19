obj-m += lt8912b.o

KERNELDIR ?=/home/chenqigan/myandroid_epc400-43.lns/kernel
PWD ?= $(shell pwd)
ARCH = arm
CROSS_COMPILE=/home/chenqigan/myandroid_epc400-43.lns/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androidkernel-

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
CFLAGS_MODULE=-fno-pic

.PHONY: modules clean
modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) $@
clean:
	rm *.o *.mod.c *.order *.symvers *.ko
