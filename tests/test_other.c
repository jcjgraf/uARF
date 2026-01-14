/**
 * Test various functionality that has not been tested elsewhere.
 */

#include "lib.h"
#include "log.h"
#include "test.h"
#include <stdio.h>

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_TEST
#endif

/**
 * Test the rounding functions
 */
UARF_TEST_CASE(round) {
    uarf_assert(UARF_ROUND_DOWN_4K(0) == 0);
    uarf_assert(UARF_ROUND_DOWN_4K(5) == 0);
    uarf_assert(UARF_ROUND_DOWN_4K(PAGE_SIZE - 1) == 0);
    uarf_assert(UARF_ROUND_DOWN_4K(PAGE_SIZE) == PAGE_SIZE);
    uarf_assert(UARF_ROUND_DOWN_4K(PAGE_SIZE + 1) == PAGE_SIZE);
    uarf_assert(UARF_ROUND_DOWN_4K(PAGE_SIZE + (PAGE_SIZE - 1)) == PAGE_SIZE);
    uarf_assert(UARF_ROUND_DOWN_4K(PAGE_SIZE + PAGE_SIZE + 1) == 2 * PAGE_SIZE);

    uarf_assert(UARF_ROUND_UP_4K(0) == 0);
    uarf_assert(UARF_ROUND_UP_4K(5) == PAGE_SIZE);
    uarf_assert(UARF_ROUND_UP_4K(PAGE_SIZE - 1) == PAGE_SIZE);
    uarf_assert(UARF_ROUND_UP_4K(PAGE_SIZE) == PAGE_SIZE);
    uarf_assert(UARF_ROUND_UP_4K(PAGE_SIZE + 1) == 2 * PAGE_SIZE);
    uarf_assert(UARF_ROUND_UP_4K(PAGE_SIZE + (PAGE_SIZE - 1)) == 2 * PAGE_SIZE);
    uarf_assert(UARF_ROUND_UP_4K(PAGE_SIZE + PAGE_SIZE + 1) == 3 * PAGE_SIZE);

    uarf_assert(UARF_ROUND_DOWN_2M(0) == 0);
    uarf_assert(UARF_ROUND_DOWN_2M(5) == 0);
    uarf_assert(UARF_ROUND_DOWN_2M(PAGE_SIZE_2M - 1) == 0);
    uarf_assert(UARF_ROUND_DOWN_2M(PAGE_SIZE_2M) == PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_DOWN_2M(PAGE_SIZE_2M + 1) == PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_DOWN_2M(PAGE_SIZE_2M + (PAGE_SIZE_2M - 1)) == PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_DOWN_2M(PAGE_SIZE_2M + PAGE_SIZE_2M + 1) == 2 * PAGE_SIZE_2M);

    uarf_assert(UARF_ROUND_UP_2M(0) == 0);
    uarf_assert(UARF_ROUND_UP_2M(5) == PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_UP_2M(PAGE_SIZE_2M - 1) == PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_UP_2M(PAGE_SIZE_2M) == PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_UP_2M(PAGE_SIZE_2M + 1) == 2 * PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_UP_2M(PAGE_SIZE_2M + (PAGE_SIZE_2M - 1)) == 2 * PAGE_SIZE_2M);
    uarf_assert(UARF_ROUND_UP_2M(PAGE_SIZE_2M + PAGE_SIZE_2M + 1) == 3 * PAGE_SIZE_2M);

    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {
    UARF_TEST_RUN_CASE(round);

    return 0;
}
