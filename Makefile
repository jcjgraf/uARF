uARF_ROOT := $(abspath $(CURDIR))

CONFIG := $(uARF_ROOT)/.config
include $(CONFIG)

ifeq ($(PLATFORM),)
	$(error Please define PLATFORM)
endif

PLATFORM_CONFIG := $(uARF_ROOT)/config/$(PLATFORM).config
include $(PLATFORM_CONFIG)

uARF_SRC := $(uARF_ROOT)/src
uARF_INCL := $(uARF_ROOT)/include
uARF_TEST := $(uARF_ROOT)/tests
uARF_KMOD := $(uARF_ROOT)/kmods

CC := gcc
AR := ar

COMMON_INCLUDES := -I$(uARF_INCL) -I$(uARF_KMOD)

# ifdef KTF_ROOT
# export __KTF__
# COMMON_INCLUDES += -I$(KTF_ROOT)/include
# endif

COMMON_FLAGS := $(COMMON_INCLUDES) -MP -MMD
AFLAGS := $(COMMON_FLAGS) -D__ASSEMBLY__
CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -g -static

SOURCES     := $(shell find $(uARF_SRC) -name \*.c)
HEADERS     := $(shell find $(uARF_INCL) $(uARF_KMOD) -name \*.h)
ASM_SOURCES := $(shell find $(uARF_SRC) $(uARF_INCL) -name \*.S)

OBJS := $(SOURCES:%.c=%.o)
OBJS += $(ASM_SOURCES:%.S=%.o)

LIBRARY_NAME := uarf
LIBRARY := lib$(LIBRARY_NAME).a

# LDFLAGS := -L $(uARF_ROOT) -l$(LIBRARY_NAME)

ifneq ($(TESTCASE),)
TESTCASE_BASE := $(uARF_TEST)/$(TESTCASE)
TESTCASE_BIN := $(TESTCASE).bin
TESTCASE_C := $(TESTCASE_BASE).c
TESTCASE_H := $(wildcard $(TESTCASE_BASE).h)
TESTCASE_ASM := $(wildcard $(TESTCASE_BASE)_asm.S)
OBJS += $(TESTCASE_C:%.c=%.o) $(TESTCASE_ASM:%.S=%.o)
endif

ifneq ($(V), 1)
	VERBOSE=@
endif

all: $(LIBRARY) kmods test

$(LIBRARY): $(OBJS)
	@echo "AR " $@
	$(VERBOSE) $(AR) rcs $@ $^

%.o: %.S
	@echo "AS " $@
	$(VERBOSE) $(CC) -c -o $@ $(AFLAGS) $<

%.o: %.c
	@echo "CC " $@
	$(VERBOSE) $(CC) -c -o $@ $(CFLAGS) $<

DEPFILES := $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))

.PHONY: test
test: $(TESTCASE_C) $(OBJS)
	@echo "CC Test " $(TESTCASE)
	$(VERBOSE) $(CC) $(CFLAGS) -o $(TESTCASE_BIN) $(filter %.o, $^)

.PHONY: clean
clean:
	@echo "Clean"
	$(VERBOSE) find $(uARF_ROOT) -name \*.d -delete
	$(VERBOSE) find $(uARF_ROOT) -name \*.o -delete
	$(VERBOSE) find $(uARF_ROOT) -name $(LIBRARY) -delete
	$(VERBOSE) find $(uARF_ROOT) -name \*.bin -delete

compile_commands_user.json: clean
	@echo "Create $@"
	$(VERBOSE) bear --output $@ -- $(MAKE) $(LIBRARY) test

compile_commands_kmods.json: kmods_clean
	@echo "Create $@"
	$(VERBOSE) bear --output $@ -- $(MAKE) kmods

.PHONY: clangd_user
clangd_user: compile_commands_user.json
	@echo "Link $< to compile_commands.json"
	rm -r compile_commands.json
	ln -s $< compile_commands.json

.PHONY: clangd_kernel
clangd_kernel: compile_commands_kmods.json
	@echo "Link $< to compile_commands.json"
	rm -r compile_commands.json
	ln -s $< compile_commands.json

.PHONY: kmods
kmods:
	@echo "Building kmods with kdir $(KDIR)"
	$(VERBOSE) $(MAKE) -C $(uARF_KMOD) KDIR=$(KDIR)

.PHONY: kmods_clean
kmods_clean:
	@echo "Clean KMOD"
	$(VERBOSE) $(MAKE) -C $(uARF_KMOD) KDIR=$(KDIR) clean
