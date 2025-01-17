/**
 * Kernel Module RAP (Run as Privileged)
 *
 * Test the RAP kernel module.
 */

#include "compiler.h"
#include "test.h"
#include "uarf_rap/uarf_rap.h"

#define ROUNDS 100

static void basic_run(void *data) {
    asm volatile("nop\n\t");
}

TEST_CASE(basic) {
    rap_init();

    rap_call(basic_run, NULL);

    rap_deinit();
    TEST_PASS();
}

static void argument_run(void *data) {
    uint64_t *a = (uint64_t *) data;
    *a = 7;
}

TEST_CASE(argument) {
    rap_init();

    uint64_t a = 5;

    rap_call(argument_run, &a);

    TEST_ASSERT(a == 7);

    rap_deinit();
    TEST_PASS();
}

TEST_SUITE() {
    RUN_TEST_CASE(basic);
    RUN_TEST_CASE(argument);

    return 0;
}
