PWD := $(CURDIR)

MODULES := pi rap smm

.DEFAULT_GOAL := all
.PHONY: all host guest clean modules_install

.PHONY: all
all: host

.PHONY: host
host:
	@echo "Building kmods for host"
	@for m in $(MODULES); do \
		$(MAKE) -C $$m host || exit $$?; \
	done

.PHONY: guest
guest:
	@echo "Building kmods for guest"
	@for m in $(MODULES); do \
		$(MAKE) -C $$m guest || exit $$?; \
	done

.PHONY: clean
clean:
	@echo "Cleaning kmods"
	@for m in $(MODULES); do \
		$(MAKE) -C $$m clean || exit $$?; \
	done

.PHONY: modules_install
modules_install: host
	@echo "Installing kmods on host"
	for m in $(MODULES); do \
	    sudo $(MAKE) -C $$m host modules_install; \
	done

PHONY: modules_uninstall
modules_uninstall:
	@echo "Unloading kmods"
	@for m in $(MODULES); do \
	    sudo $(MAKE) -C $$m modules_uninstall; \
	done
