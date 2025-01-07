/*
 * Name: test_kmod_msr
 * Desc: Test the msr kernel module
 */

#include "test.h"
#include "uarf_msr/uarf_msr.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/" DEVICE_NAME

TEST_CASE(read_write) {

    msr_init();

    uint64_t val_orig = msr_rdmsr(0x48);

    uint64_t val_new = val_orig ^ 1;
    msr_wrmsr(0x48, val_new);

    uint64_t val_changed = msr_rdmsr(0x48);

    TEST_ASSERT((val_changed ^ 1) == val_orig);

    msr_wrmsr(0x48, val_orig);

    uint64_t val_final = msr_rdmsr(0x48);

    TEST_ASSERT(val_orig == val_final);

    TEST_PASS();
}

TEST_SUITE() {
    RUN_TEST_CASE(read_write);

    return 0;
}
