ifneq ($(KERNELRELEASE),)
obj-m := my_usb.o
 #Otherwise we were called directly from the command
 # line; invoke the kernel build system.
else
 KERNELDIR ?= /lib/modules/3.13.0-32-generic/build/ 
 PWD := $(shell pwd)
clean:
	rm -rf *.o *.ko
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
