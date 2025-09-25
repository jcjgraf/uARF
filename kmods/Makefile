PWD = $(CURDIR)

MODULES := pi rap

KDIR ?= /lib/modules/$(shell uname -r)/build

KDIR_HOST := $(KDIR)
KDIR_GUEST := $(firstword $(wildcard \
		/home/jeanclaude/Documents/Programming/LinuxKernel/Mainline \
		/local/home/jegraf/LinuxKernelTorvalds))

# Since we change the KDIR variable, we need to re-compile every time.
# For this, we define it as PHONY and clean before the compilation
define rule_template_host =
.PHONY: $(1)-host.ko
$(1)-host.ko:
	@echo "Build $$@"
	# KDIR=$$(KDIR_HOST)
	$(MAKE) -C $$(KDIR_HOST) M=$$(abspath $(1)) clean
	$(MAKE) -C $$(KDIR_HOST) M=$$(abspath $(1))
	@echo "Copy $(1)/$(1).ko as $$@"
	cp $(1)/$(1).ko $$@
TARGET_HOST += $(1)-host.ko
endef

define rule_template_guest =
.PHONY: $(1)-guest.ko
$(1)-guest.ko:
	@echo "Build $$@"
	# KDIR=$$(KDIR_GUEST)
	$(MAKE) -C $$(KDIR_GUEST) M=$$(abspath $(1)) clean
	$(MAKE) -C $$(KDIR_GUEST) M=$$(abspath $(1))
	@echo "Copy $(1)/$(1).ko as $$@"
	cp $(1)/$(1).ko $$@
TARGET_GUEST += $(1)-guest.ko
endef

$(foreach m,$(MODULES),$(eval $(call rule_template_host,$(m))))
$(foreach m,$(MODULES),$(eval $(call rule_template_guest,$(m))))

# Need to have this after the eval, as otherwise the variables (TARGET etc.) are not populated
.DEFAULT_GOAL:=all
.PHONY: all
all: $(TARGET_HOST) $(TARGET_GUEST)

.PHONY: host
host: $(TARGET_HOST)
	@echo "Build for host"

.PHONY: guest
guest: $(TARGET_GUEST)
	@echo "Build for guest"

%.ko: %.c
	@echo "Building kernel module $@"
	$(MAKE) -C $(KDIR) M=$(abspath $(dir $@))

.PHONY: clean
clean:
	@echo "Cleaning"
	@for m in $(MODULES); do \
	    $(MAKE) -C $(KDIR_HOST) M=$(abspath $$m) clean; \
	    $(MAKE) -C $(KDIR_GUEST) M=$(abspath $$m) clean; \
	done
	rm -rf $(TARGET_HOST) $(TARGET_GUEST)


# # Host staging
# # %-host.ko: KDIR=$(KDIR_HOST)
# %-host.ko: %/%.ko
# 	@echo "Staging $< as $@ (host)"
# 	cp $< $@
#
# # Guest staging
# %-guest.ko: %/%.ko
# 	@echo "Staging $< as $@ (guest)"
# 	cp $< $@

# %.ko: %.c
# 	@echo "Building kernel module $@"
# 	$(MAKE) -C $(KDIR) M=$(abspath $(dir $@))
#
# %-host.ko: %/%.ko
# 	@echo "Building $@ for host"
# 	cp $< $@
#
# %-guest.ko: %/%.ko
# 	@echo "Building $@ for guest"
# 	cp $< $@

