PWD = $(CURDIR)

ifeq ($(KDIR),)
$(error Do not run this makefile directly, but via its parent)
endif

all: uarf_msr/uarf_msr.ko

uarf_msr/uarf_msr.ko: uarf_msr/uarf_msr.c
	@echo "Building kernel module in $@"
	$(MAKE) -C $(KDIR) M=$(PWD)/uarf_msr

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/uarf_msr clean
