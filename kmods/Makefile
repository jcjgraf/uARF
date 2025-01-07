PWD = $(CURDIR)

ifeq ($(KDIR),)
$(error Do not run this makefile directly, but via its parent)
endif

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
