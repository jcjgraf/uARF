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
#include "uarch.h"

#include <sys/io.h>

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_TEST
#endif

/**
 * Trigger and count SMI using kernel module
 *
 * Run 3 out instructions via the kernel module and verify that they triggered
 * an SMI by reading the SMI count MSR/PFC.
 *
 * Intel counts SMIs using a MSR, AMD using a PFC.
 */
UARF_TEST_CASE(basic) {
    uarf_pi_init();

#if UARF_IS_INTEL()
    uint64_t start = uarf_rdmsr_user(MSR_SMI_COUNT);
#elif UARF_IS_AMD()
    UarfPfc pfc;
    uarf_pfc_init(&pfc, UARF_AMD_LS_SMI_RX, 0);
    uarf_pfc_reset(&pfc);
    uarf_pfc_start(&pfc);
#endif

    uarf_out_kmod(0xb2, 0x0);
    uarf_out_kmod(0xb2, 0x0);
    uarf_out_kmod(0xb2, 0x0);

    uint64_t val;

#if UARF_IS_INTEL()
    val = uarf_rdmsr_user(MSR_SMI_COUNT) - start;
#elif UARF_IS_AMD()
    uarf_pfc_stop(&pfc);
    val = uarf_pfc_read(&pfc);
    uarf_pfc_deinit(&pfc);
#endif

    printf("%lu\n", val);
    UARF_TEST_ASSERT(val == 3);

    uarf_pi_deinit();
    UARF_TEST_PASS();
}

/**
 * Repeatedly trigger SMIs.
 *
 * Can be measured in a different process. Does not work for AMD with the PFC,
 * as PFCs are local to the process.
 */
UARF_TEST_CASE(trigger) {
    uarf_pi_init();

    while (1) {
        uarf_out_kmod(0xb2, 0x0);
        sleep(1);
    }

    UARF_TEST_PASS();
}

/**
 * Iteratively measure SMIs.
 */
UARF_TEST_CASE(signal) {
    uarf_pi_init();

    // uint64_t start = uarf_rdmsr_user(MSR_SMI_COUNT);
#if UARF_IS_INTEL()
    uint64_t start = uarf_rdmsr_user(MSR_SMI_COUNT);
#elif UARF_IS_AMD()
    UarfPfc pfc;
    uarf_pfc_init(&pfc, UARF_AMD_LS_SMI_RX, 0);
    uarf_pfc_reset(&pfc);
    uarf_pfc_start(&pfc);
#endif

    uint64_t val;

    while (1) {
#if UARF_IS_INTEL()
        val = uarf_rdmsr_user(MSR_SMI_COUNT) - start;
#elif UARF_IS_AMD()
        uarf_pfc_stop(&pfc);
        val = uarf_pfc_read(&pfc);
#endif
        printf("%lu\n", val);
        sleep(1);
    }

#if UARF_IS_AMD()
    uarf_pfc_deinit(&pfc);
#endif
    uarf_pi_deinit();
    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {

    UARF_TEST_RUN_CASE(basic);
    // UARF_TEST_RUN_CASE(intel_basic_supervisor);
    // UARF_TEST_RUN_CASE(trigger);
    // UARF_TEST_RUN_CASE(signal);
    return 0;
}
