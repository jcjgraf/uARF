/**
 * Play with performance counters and show how we can interact with them.
 */

#include "lib.h"
#include "pfc.h"
#include "pfc_amd.h"
#include "test.h"

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_TEST
#endif

UARF_TEST_CASE(read) {
    UarfPfc pfc = {.config = UARF_AMD_EX_RET_INSTR};

    uarf_pfc_init(&pfc);

    uint64_t start;
    read(pfc.fd, &start, 8);
    ({
        volatile uint64_t a = 5;
        for (size_t i = 0; i < 10; i++) {
            a *= 3;
        }
    });
    uint64_t end;
    read(pfc.fd, &end, 8);

    printf("%lu\n", end - start);

    uarf_pfc_deinit(&pfc);
    UARF_TEST_PASS();
}

UARF_TEST_CASE(rdpmc) {
    UarfPfc pfc = {.config = UARF_AMD_EX_RET_INSTR};

    uarf_pfc_init(&pfc);

    uint64_t start = uarf_pfc_read(&pfc);
    ({
        volatile uint64_t a = 5;
        for (size_t i = 0; i < 10; i++) {
            a *= 3;
        }
    });
    uint64_t end = uarf_pfc_read(&pfc);

    printf("%lu\n", end - start);

    uarf_pfc_deinit(&pfc);
    UARF_TEST_PASS();
}

UARF_TEST_CASE(pm) {
    UarfPm measure;

    // pm_init(&measure, AMD_EX_RET_INSTR);
    uarf_pm_init(&measure, UARF_AMD_EX_RET_BRN_IND_MISP);

    uarf_mfence();
    uarf_lfence();
    uarf_pm_start(&measure);
    uarf_pm_stop(&measure);

    printf("%lu\n", uarf_pm_get(&measure));

    uarf_pm_reset(&measure);
    uarf_mfence();
    uarf_lfence();
    uarf_pm_start(&measure);
    uarf_pm_stop(&measure);

    printf("%lu\n", uarf_pm_get(&measure));

    uarf_pm_deinit(&measure);

    UARF_TEST_PASS();
}

UARF_TEST_CASE(pmg) {
    UarfPmg pmg;

    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {
    UARF_TEST_RUN_CASE(read);
    UARF_TEST_RUN_CASE(rdpmc);
    UARF_TEST_RUN_CASE(pm);
    UARF_TEST_RUN_CASE(pmg);
    return 0;
}
