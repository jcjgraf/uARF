uARF_ROOT := $(abspath $(CURDIR))

CONFIG := $(uARF_ROOT)/.config
include $(CONFIG)

uARF_SRC := $(uARF_ROOT)/src
uARF_INCL := $(uARF_ROOT)/include
uARF_TEST := $(uARF_ROOT)/tests
uARF_KMOD := $(uARF_ROOT)/kmods

CC := gcc
AR := ar

COMMON_DEFINE := -DUARCH=$(UARCH)
COMMON_INCLUDES := -I$(uARF_INCL) -I$(uARF_KMOD)/smm/include

COMMON_FLAGS := $(COMMON_INCLUDES) $(COMMON_DEFINE) -MP -MMD
AFLAGS := $(COMMON_FLAGS) -D__ASSEMBLY__
# WARNING Changing the optimisation leads to different overheads of the measurement function
CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -g -static -O3

SOURCES     := $(shell find $(uARF_SRC) -name \*.c)
# HEADERS     := $(shell find $(uARF_INCL) $(uARF_KMOD) -name \*.h)
ASM_SOURCES := $(shell find $(uARF_SRC) $(uARF_INCL) -name \*.S)

OBJS := $(SOURCES:%.c=%.o)
OBJS += $(ASM_SOURCES:%.S=%.o)

LIBRARY_NAME := uarf
LIBRARY := lib$(LIBRARY_NAME).a

LIBRARY_PREFIX = /usr/local
LIBRARY_INCLUDE_DIR = $(LIBRARY_PREFIX)/include/$(LIBRARY_NAME)
LIBRARY_LIB_DIR = $(LIBRARY_PREFIX)/lib

# LDFLAGS := -L $(uARF_ROOT) -l$(LIBRARY_NAME)

ifneq ($(TESTCASE),)
TESTCASE_BASE := $(uARF_TEST)/$(TESTCASE)
TESTCASE_BIN := $(notdir $(TESTCASE)).bin
TESTCASE_C := $(TESTCASE_BASE).c
TESTCASE_H := $(wildcard $(TESTCASE_BASE).h)
TESTCASE_ASM := $(wildcard $(TESTCASE_BASE)_asm.S)
TESTCASE_32 := $(wildcard $(TESTCASE_BASE)_32.c)
TESTCASE_OBJS := $(TESTCASE_C:%.c=%.o) $(TESTCASE_ASM:%.S=%.o) $(TESTCASE_32:%.c=%.o)
endif

ifneq ($(V), 1)
	VERBOSE=@
endif

all: $(LIBRARY) kmods test

$(LIBRARY): $(OBJS)
	@echo "AR " $@
	$(VERBOSE) $(AR) rcs $@ $^

%_32.o: %_32.c
	@echo "CC 32" $@
	$(VERBOSE) $(CC) -m32 -c -o $@.tmp $(CFLAGS) -fno-stack-protector -fno-pic -ffreestanding $<
	$(VERBOSE) objcopy -I elf32-i386 -O elf64-x86-64 $@.tmp $@
	$(VERBOSE) rm $@.tmp

%.o: %.S
	@echo "AS " $@
	$(VERBOSE) $(CC) -c -o $@ $(AFLAGS) $<

%.o: %.c
	@echo "CC " $@
	$(VERBOSE) $(CC) -c -o $@ $(CFLAGS) $<

DEPFILES := $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))

.PHONY: test
test: $(TESTCASE_C) $(OBJS) $(TESTCASE_OBJS)
	@echo "CC Test " $(TESTCASE)
	$(VERBOSE) $(CC) $(CFLAGS) -o $(TESTCASE_BIN) $(filter %.o, $^)

.PHONY: clean
clean:
	@echo "Clean"
	$(VERBOSE) find $(uARF_ROOT) -name \*.d -delete
	$(VERBOSE) find $(uARF_ROOT) -name \*.o -delete
	$(VERBOSE) find $(uARF_ROOT) -name $(LIBRARY) -delete
	$(VERBOSE) find $(uARF_ROOT) -name \*.bin -delete

.PHONY: kmods
kmods:
	@echo "Building kmods"
	$(VERBOSE) $(MAKE) -C $(uARF_KMOD) all

.PHONY: kmods_clean
kmods_clean:
	@echo "Clean kmods"
	$(VERBOSE) $(MAKE) -C $(uARF_KMOD) clean

.PHONY: kmods_install
kmods_install:
	@echo "Install kmods"
	$(VERBOSE) $(MAKE) -C $(uARF_KMOD) modules_install

.PHONY: kmods_uninstall
kmods_uninstall:
	@echo "Uninstall kmods"
	$(VERBOSE) $(MAKE) -C $(uARF_KMOD) modules_uninstall

# Install library
.PHONY: install
install: $(LIBRARY) uninstall
	@echo "Install library"
	$(VERBOSE) install -d -m 755 $(LIBRARY_LIB_DIR) $(LIBRARY_INCLUDE_DIR)/kmod
	$(VERBOSE) install -m 644 $(LIBRARY) $(LIBRARY_LIB_DIR)/
	$(VERBOSE) install -m 644 -D include/*.h $(LIBRARY_INCLUDE_DIR)/
	$(VERBOSE) install -m 644 -D include/kmod/*.h $(LIBRARY_INCLUDE_DIR)/kmod


# Uninstall library
.PHONY: uninstall
uninstall:
	@echo "Uninstall library"
	$(VERBOSE) rm -f $(LIBRARY_LIB_DIR)/$(LIBRARY)
	$(VERBOSE) rm -rf $(LIBRARY_INCLUDE_DIR)
