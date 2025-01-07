ccflags-y := \
	-DDEBUG \
	$(CCFLAGS)

PWD = $(CURDIR)

# TODO: Set depending on target
# KDIR := /lib/modules/$(shell uname -r)/build
KDIR := /home/jeanclaude/Documents/Programming/LinuxKernelTorvalds

all: rdmsr/rdmsr.ko

rdmsr/rdmsr.ko: rdmsr/rdmsr.c
	@echo "Building kernel module in $@"
	$(MAKE) -C $(KDIR) M=$(PWD)/rdmsr

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/rdmsr clean

.PHONY: load
load:
	@echo "Load"
	sudo insmod $(PWD)/rdmsr/rdmsr.ko

.PHONY: kmod_unload
kmod_unload:
	@echo "Unload kmod"
	sudo rmmod $(PWD)/rdmsr/rdmsr.ko

.PHONY: kmod_reload
kmod_reload:
	@echo "Reload kmod"
	unload
	load
