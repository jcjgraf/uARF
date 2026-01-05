/**
 * Test kernel module SMM.
 */

#include "jita.h"
#include "lib.h"
#include "psnip.h"
#include "smm.h"
#include "stub.h"
#include "test.h"

// Test the UARF_SMM_IOCTL_PING ioctl
UARF_TEST_CASE(ping) {

    uarf_assert(!uarf_smm_open());

    uint64_t v0 = 0x4;
    uint64_t v1 = uarf_smm_ping(v0);

    printf("This should be 1: 0x%lx\n", v1 - v0);
    uarf_assert(v1 - v0 == 1);

    uarf_assert(!uarf_smm_close());

    UARF_TEST_PASS();
}

uarf_psnip_declare_define(psnip_ret, "ret\n\t");

// Test the UARF_SMM_IOCTL_REGISTER and _RUN ioctl
UARF_TEST_CASE(custom_handler) {
    uarf_assert(!uarf_smm_open());

    UarfJitaCtxt jita = uarf_jita_init();
    UarfStub stub = uarf_stub_init();
    uarf_jita_push_psnip(&jita, &psnip_ret);
    uarf_jita_allocate(&jita, &stub, uarf_rand47());

    uarf_assert(
        !uarf_smm_register(stub.base_addr, stub.end_addr - stub.base_addr));

    uarf_assert(!uarf_smm_run());
    printf("We made it!\n");

    uarf_assert(!uarf_smm_close());
    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {

    // uarf_log_system_base_level = UARF_LOG_LEVEL_DEBUG;

    UARF_TEST_RUN_CASE(ping);
    UARF_TEST_RUN_CASE(custom_handler);
}
