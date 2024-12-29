uARF_ROOT := $(abspath $(CURDIR))

CONFIG := $(uARF_ROOT)/.config
include $(CONFIG)

CC := gcc
AR := ar

COMMON_INCLUDES := -I$(uARF_ROOT)/include

# ifdef KTF_ROOT
# export __KTF__
# COMMON_INCLUDES += -I$(KTF_ROOT)/include
# endif

COMMON_FLAGS := $(COMMON_INCLUDES) -MP -MMD
AFLAGS := $(COMMON_FLAGS) -D__ASSEMBLY__
CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -g

SOURCES     := $(shell find . -name \*.c -and -not -path "./tests/test_*")
HEADERS     := $(shell find . -name \*.h)
ASM_SOURCES := $(shell find . -name \*.S)

OBJS := $(SOURCES:%.c=%.o)
OBJS += $(ASM_SOURCES:%.S=%.o)

LIBRARY_NAME := uarf
LIBRARY := lib$(LIBRARY_NAME).a

# LDFLAGS := -L $(uARF_ROOT) -l$(LIBRARY_NAME)

TEST_SOURCES := $(shell find $(TEST_DIR) -name "test_*.c")
TEST_TARGETS := $(patsubst $(TEST_DIR)/%.c, %, $(TEST_SOURCES))

ifneq ($(TESTCASE),)
TEST_DIR := ./tests
TESTCASE_BASE := ./tests/$(TESTCASE)
TESTCASE_BIN := $(TESTCASE).bin
TESTCASE_C := $(TESTCASE_BASE).c
TESTCASE_H := $(wildcard $(TESTCASE_BASE).h)
TESTCASE_ASM := $(wildcard $(TESTCASE_BASE)_asm.S)
OBJS += $(TESTCASE_C:%.c=%.o) $(TESTCASE_ASM:%.S=%.o)
endif

ifneq ($(V), 1)
	VERBOSE=@
endif

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

list_tests:
	@echo "Available tests:"
	$(VERBOSE) for test in $(TEST_TARGETS); do echo "  $$test"; done

.PHONY: clean
clean:
	@echo "Clean"
	$(VERBOSE) find $(uARF_ROOT) -name \*.d -delete
	$(VERBOSE) find $(uARF_ROOT) -name \*.o -delete
	$(VERBOSE) find $(uARF_ROOT) -name $(LIBRARY) -delete
	$(VERBOSE) find $(uARF_ROOT) -name \*.bin -delete
