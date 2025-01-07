/*
 * Name: test_kmod_rdmsr
 * Desc: Rest the rdmsr kernel module
 */

#include "rdmsr/rdmsr.h"
#include "test.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/" DEVICE_NAME

int main(void) {

    msr_init();

    uint64_t val_orig = msr_rdmsr(0x48);
    printf("Original value: 0x%lx\n", val_orig);

    uint64_t val_new = val_orig ^ 1;
    msr_wrmsr(0x48, val_new);

    uint64_t val_changed = msr_rdmsr(0x48);
    printf("Changed value: 0x%lx, should be 0x%lx\n", val_changed, val_new);

    msr_wrmsr(0x48, val_orig);

    uint64_t val_final = msr_rdmsr(0x48);
    printf("Reset value: 0x%lx\n", val_final);

    return 0;
}
