# <kernel_sources>/Documentation/kbuild/modules.txt.

KERNELRELEASE ?= $(shell uname -r)
KERNELDIR := /lib/modules/$(KERNELRELEASE)/build

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	-rm include/linux/usb # tar cannot compress if symlink is dangling

