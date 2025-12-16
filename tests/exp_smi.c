/**
 * Play with the SMM.
 */

#include "jita.h"
#include "lib.h"
#include "msr_intel.h"
#include "pfc.h"
#include "pfc_amd.h"
#include "spec_lib.h"
#include "test.h"

#include <sys/io.h>

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_TEST
#endif

/**
 * Trigger and count SMI using kernel module (AMD only)
 *
 * Try to trigger SMI and detect using kernel module. But we get no signal.
 */
// UARF_TEST_CASE(amd_basic) {
//     uarf_pi_init();
//
//     UarfPfc pfc;
//     uarf_pfc_init(&pfc, UARF_AMD_LS_SMI_RX);
//
//     uarf_pfc_reset(&pfc);
//     uarf_pfc_start(&pfc);
//
//     // for (size_t i = 0; i < 0xffffffff; i++) {
//     //     // if (!i % 0xff) {
//     //     uint64_t val = uarf_pfc_read(&pfc);
//     //     UARF_LOG_INFO("%lu, %lu\n", i, val);
//     //     // }
//     //     barf_pi_out(0x0, i);
//     // }
//
//     while (1) {
//         sleep(10);
//         uint64_t val = uarf_pfc_read(&pfc);
//
//         printf("%lu\n", val);
//     }
//
//     uarf_pfc_stop(&pfc);
//     uint64_t val = uarf_pfc_read(&pfc);
//
//     printf("%lu\n", val);
//
//     uarf_pfc_deinit(&pfc);
//
//     uarf_pi_deinit();
//     UARF_TEST_PASS();
// }

/**
 * Trigger and count SMI using kernel module (Intel only)
 *
 * Run 3 out instructions via the kernel module and verify that they triggered
 * an SMI by reading the SMI count MSR.
 */
UARF_TEST_CASE(intel_basic) {
    uarf_pi_init();

    uint64_t start = uarf_rdmsr_user(MSR_SMI_COUNT);

    uarf_out_kmod(0xb2, 0x0);
    uarf_out_kmod(0xb2, 0x0);
    uarf_out_kmod(0xb2, 0x0);

    uint64_t val = uarf_rdmsr_user(MSR_SMI_COUNT) - start;

    printf("%lu\n", val);
    UARF_TEST_ASSERT(val == 3);

    uarf_pi_deinit();
    UARF_TEST_PASS();
}

/**
 * Trigger and count SMI in CPL (Intel only)
 *
 * Run 3 `out` instructions in the CPL and verify that they triggered an SMI by
 * reading the SMI count MSR.
 */
UARF_TEST_CASE(intel_basic_supervisor) {
    uarf_pi_init();

    uint64_t start = uarf_rdmsr(MSR_SMI_COUNT);

    uarf_out(0xb2, 0x0);
    uarf_out(0xb2, 0x0);
    uarf_out(0xb2, 0x0);

    uint64_t val = uarf_rdmsr(MSR_SMI_COUNT) - start;

    printf("%lu\n", val);
    UARF_TEST_ASSERT(val == 3);

    uarf_pi_deinit();
    UARF_TEST_PASS();
}

/**
 * Repeatedly trigger SMIs.
 *
 * Can be measured in a different process.
 */
UARF_TEST_CASE(intel_trigger) {
    uarf_pi_init();

    while (1) {
        uarf_pi_out(0x0, 0xb2);
        sleep(1);
    }

    UARF_TEST_PASS();
}

/**
 * Iteratively measure SMIs.
 */
UARF_TEST_CASE(intel_signal) {
    uarf_pi_init();
    uint64_t start = uarf_rdmsr_user(MSR_SMI_COUNT);

    while (1) {
        uint64_t val = uarf_rdmsr_user(MSR_SMI_COUNT) - start;
        printf("%lu\n", val);
        sleep(1);
    }
    uarf_pi_deinit();
    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {
    // UARF_TEST_RUN_CASE(amd_basic);
    UARF_TEST_RUN_CASE(intel_basic);
    // UARF_TEST_RUN_CASE(intel_basic_supervisor);
    // UARF_TEST_RUN_CASE(intel_trigger);
    // UARF_TEST_RUN_CASE(intel_signal);
    return 0;
}
