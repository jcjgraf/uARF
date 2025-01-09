PWD = $(CURDIR)

ifeq ($(KDIR),)
$(error Do not run this makefile directly, but via its parent)
endif

all: uarf_pi/uarf_pi.ko

uarf_pi/uarf_pi.ko: uarf_pi/uarf_pi.c
	@echo "Building kernel module in $@"
	$(MAKE) -C $(KDIR) M=$(PWD)/uarf_pi

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/uarf_pi clean
