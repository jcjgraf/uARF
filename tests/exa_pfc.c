/**
 * Play with performance counters and show how we can interact with them.
 */

#include "pfc.h"
#include "pfc_amd.h"
#include "test.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_TEST
#endif

TEST_CASE(read) {
    struct pfc pfc = {.config = AMD_EX_RET_INSTR};

    pfc_init(&pfc);

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
    TEST_PASS();
}

TEST_CASE(rdpmc) {
    struct pfc pfc = {.config = AMD_EX_RET_INSTR};

    pfc_init(&pfc);

    uint64_t start = pfc_read(&pfc);
    ({
        volatile uint64_t a = 5;
        for (size_t i = 0; i < 10; i++) {
            a *= 3;
        }
    });
    uint64_t end = pfc_read(&pfc);

    printf("%lu\n", end - start);
    TEST_PASS();
}

TEST_SUITE() {
    RUN_TEST_CASE(read);
    RUN_TEST_CASE(rdpmc);
    return 0;
}
