/**
 * Test kernel module SMM.
 */

#include "lib.h"
#include "smm.h"
#include "test.h"

// Test the UARF_SMM_IOCTL_PING ioctl
UARF_TEST_CASE(ping) {

    uarf_assert(!uarf_smm_open());

    uint64_t v0 = 0x4;
    uint64_t v1 = uarf_smm_ping(v0);

    printf("0x%lx\n", v1);
    uarf_assert(v1 - v0 == 1);

    uarf_assert(!uarf_smm_close());

    UARF_TEST_PASS();
}

// Test the UARF_SMM_IOCTL_REGISTER and _RUN ioctls
UARF_TEST_CASE(custom_handler) {
    uarf_assert(!uarf_smm_open());

    uarf_assert(!uarf_smm_close());
    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {
    UARF_TEST_RUN_CASE(ping);
    UARF_TEST_RUN_CASE(custom_handler);
}
