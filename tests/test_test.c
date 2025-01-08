/*
 * Name: test_test
 * Desc: Is the test setup (runner) working?
 */

#include "test.h"
#include <stdio.h>

TEST_CASE(hello_world) {
    printf("Hello, World\n");
    TEST_PASS();
}

TEST_CASE_ARG(hello_arg, arg) {
    TEST_ASSERT(_ul(arg) == 5);
    TEST_PASS();
}

TEST_SUITE() {
    RUN_TEST_CASE(hello_world);
    RUN_TEST_CASE_ARG(hello_arg, 5);
}
