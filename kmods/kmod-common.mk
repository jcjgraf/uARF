KDIR_HOST ?= /lib/modules/$(shell uname -r)/build

# Optional guest kernel tree (first that exists)
KDIR_GUEST ?= $(firstword $(wildcard \
	 /home/jeanclaude/Documents/Programming/LinuxKernel/Mainline \
	 /local/home/jegraf/LinuxKernelTorvalds))

PWD := $(shell pwd)

# Default: build for host
.PHONY: all
	all: host

.PHONY: host
host:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) modules

.PHONY: guest
guest:
ifeq ($(KDIR_GUEST),)
	$(error No guest kernel tree configured (KDIR_GUEST empty))
else
	$(MAKE) -C $(KDIR_GUEST) M=$(PWD) modules
endif

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) clean
ifneq ($(KDIR_GUEST),)
	$(MAKE) -C $(KDIR_GUEST) M=$(PWD) clean
endif
