/**
 * Kernel Module PI (Privileged Instruction)
 *
 *
 */

#include "lib.h"
#include "mem.h"
#include "test.h"
#include "kmod/pi.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/" DEVICE_NAME
#define ROUNDS 100

TEST_CASE(rdmsr_wrmsr) {

    pi_init();

    uint64_t val_orig = pi_rdmsr(0x48);

    uint64_t val_new = val_orig ^ 1;
    pi_wrmsr(0x48, val_new);

    uint64_t val_changed = pi_rdmsr(0x48);

    TEST_ASSERT((val_changed ^ 1) == val_orig);

    pi_wrmsr(0x48, val_orig);

    uint64_t val_final = pi_rdmsr(0x48);

    TEST_ASSERT(val_orig == val_final);

    pi_deinit();

    TEST_PASS();
}

TEST_CASE(invlpg) {
    pi_init();
    int *loc = malloc_or_die(sizeof(int));

    uint64_t t_c = 0;
    uint64_t t_n = 0;

    for (size_t i = 0; i < ROUNDS; i++) {
        *(volatile int *) loc = 0;

        mfence();

        t_c += get_access_time(loc);

        mfence();

        pi_invlpg(_ul(loc));

        mfence();

        t_n += get_access_time(loc);
    }

    t_c /= ROUNDS;
    t_n /= ROUNDS;

    LOG_DEBUG("TLB:    %lu\n", t_c);
    LOG_DEBUG("No TLB: %lu\n", t_n);

    TEST_ASSERT(t_c < t_n);

    free_or_die(loc);

    pi_deinit();

    TEST_PASS();
}

TEST_CASE(flush_tlb) {
    pi_init();
    int *loc = malloc_or_die(sizeof(int));

    uint64_t t_c = 0;
    uint64_t t_n = 0;

    for (size_t i = 0; i < ROUNDS; i++) {
        *(volatile int *) loc = 0;

        mfence();

        t_c += get_access_time(loc);

        mfence();

        pi_invlpg(_ul(loc));

        mfence();

        t_n += get_access_time(loc);
    }

    t_c /= ROUNDS;
    t_n /= ROUNDS;

    LOG_DEBUG("TLB:    %lu\n", t_c);
    LOG_DEBUG("No TLB: %lu\n", t_n);

    TEST_ASSERT(t_c < t_n);

    free_or_die(loc);

    pi_deinit();

    TEST_PASS();
}

TEST_CASE(cpuid) {
    pi_init();

    uint32_t eax, ebx, ecx, edx;

    uint32_t leaf = 0x80000021;

    cpuid(leaf, &eax, &ebx, &ecx, &edx);

    LOG_DEBUG("EAX: %x\n", eax);
    LOG_DEBUG("EBX: %x\n", ebx);
    LOG_DEBUG("ECX: %x\n", ecx);
    LOG_DEBUG("EDX: %x\n", edx);

    TEST_ASSERT(cpuid_eax(leaf) == eax);
    TEST_ASSERT(cpuid_ebx(leaf) == ebx);
    TEST_ASSERT(cpuid_ecx(leaf) == ecx);
    TEST_ASSERT(cpuid_edx(leaf) == edx);

    pi_deinit();
    TEST_PASS();
}

TEST_SUITE() {
    RUN_TEST_CASE(rdmsr_wrmsr);
    RUN_TEST_CASE(invlpg);
    RUN_TEST_CASE(flush_tlb);
    RUN_TEST_CASE(cpuid);

    return 0;
}
