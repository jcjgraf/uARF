/**
 * Play with performance counters and show how we can interact with them.
 */

#include "jita.h"
#include "lib.h"
#include "pfc.h"
#include "pfc_amd.h"
#include "pfc_intel.h"
#include "spec_lib.h"
#include "test.h"
#include "uarch.h"

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_TEST
#endif

static __noinline void func1_loop(void) {
    volatile uint64_t a = 5;
    for (size_t i = 0; i < 10; i++) {
        a *= 3;
    }

    *(volatile uint64_t *) &a;
}

#if UARF_IS_INTEL()
#define FUNC1_LOOP_INSNS_SYS   63
#define FUNC1_LOOP_INSNS_RDPMC 18 // Between 12 and 18
#elif UARF_IS_AMD()
#define FUNC1_LOOP_INSNS_SYS   65
#define FUNC1_LOOP_INSNS_RDPMC 44
#endif

/**
 * Example as copied from `man perv_event_open`.
 */
UARF_TEST_CASE(man) {
    int fd;
    long long count;
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
#if UARF_IS_INTEL()
    // pe.config = UARF_INTEL_INST_RETIRED_PREC_DIST;
    pe.config = UARF_INTEL_INST_RETIRED_ANY_P;
#elif UARF_IS_AMD()
    pe.config = UARF_AMD_EX_RET_INSTR;
#endif
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = uarf_perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        UARF_TEST_FAIL("Failed to open: %s\n", strerror(errno));
    }

    if (ioctl(fd, PERF_EVENT_IOC_RESET, 0) == -1) {
        UARF_TEST_FAIL("Failed to reset\n");
    }
    if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
        UARF_TEST_FAIL("Failed to enable\n");
    }

    func1_loop();

    if (ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
        UARF_TEST_FAIL("Failed to disable\n");
    }
    if (read(fd, &count, sizeof(count)) != sizeof(count)) {
        UARF_TEST_FAIL("Failed to read\n");
    }

    printf("count: %lld\n", count);
    UARF_TEST_ASSERT(FUNC1_LOOP_INSNS_SYS == count);

    if (close(fd) == -1) {
        UARF_TEST_FAIL("Failed to close\n");
    }

    UARF_TEST_PASS();
}

/**
 * Basic usage example, using read to extract the value.
 */
UARF_TEST_CASE(basic) {
    UarfPfc pfc;

    UarfPfcConfig config = (UarfPfcConfig) {
#if UARF_IS_INTEL()
        .pmu_conf = UARF_INTEL_INST_RETIRED_PREC_DIST,
#elif UARF_IS_AMD()
        .pmu_conf = UARF_AMD_EX_RET_INSTR,
#endif
        .exclude = UARF_PFC_EXCLUDE_KERNEL,
        .start_disabled = true};

    uarf_pfc_init(&pfc, config);

    uarf_pfc_stop(&pfc);
    uarf_pfc_reset(&pfc);
    uarf_pfc_start(&pfc);

    func1_loop();

    uarf_pfc_stop(&pfc);
    uint64_t count = uarf_pfc_read(&pfc);
    printf("count: %lu\n", count);
    UARF_TEST_ASSERT(FUNC1_LOOP_INSNS_SYS == count);

    uarf_pfc_deinit(&pfc);
    UARF_TEST_PASS();
}

/**
 * Basic usage example using the rdpmc instruction to extract the value.
 *
 * Calculate difference manually, otherwise we may get noise introduced by the
 * ioctl
 */
UARF_TEST_CASE(rdpmc) {
    UarfPfc pfc;

    UarfPfcConfig config = (UarfPfcConfig) {
#if UARF_IS_INTEL()
        .pmu_conf = UARF_INTEL_INST_RETIRED_PREC_DIST,
#elif UARF_IS_AMD()
        .pmu_conf = UARF_AMD_EX_RET_INSTR,
#endif
        .exclude = UARF_PFC_EXCLUDE_KERNEL};

    uarf_pfc_init(&pfc, config);
    uarf_pfc_reset(&pfc);
    uarf_pfc_start(&pfc);

    uint64_t start = uarf_pfc_read_rdpmc(&pfc);

    func1_loop();

    uint64_t count = uarf_pfc_read_rdpmc(&pfc) - start;

    printf("%lu\n", count);
    UARF_TEST_ASSERT(FUNC1_LOOP_INSNS_RDPMC >= count);

    uarf_pfc_deinit(&pfc);
    UARF_TEST_PASS();
}

UARF_TEST_CASE(two_pmc) {
    UarfPfc pfc;
    UarfPfc pfc2;

    UarfPfcConfig config = (UarfPfcConfig) {
#if UARF_IS_INTEL()
        .pmu_conf = UARF_INTEL_INST_RETIRED_ANY_P,
#elif UARF_IS_AMD()
        .pmu_conf = UARF_AMD_EX_RET_INSTR,
#endif
        .exclude = UARF_PFC_EXCLUDE_KERNEL};

    UarfPfcConfig config2 = (UarfPfcConfig) {
#if UARF_IS_INTEL()
        .pmu_conf = UARF_INTEL_INST_RETIRED_PREC_DIST,
#elif UARF_IS_AMD()
        .pmu_conf = UARF_AMD_EX_RET_INSTR,
#endif
        .exclude = UARF_PFC_EXCLUDE_KERNEL};

    uarf_pfc_init(&pfc, config);
    uarf_pfc_init(&pfc2, config2);

    uarf_pfc_reset(&pfc);
    uarf_pfc_reset(&pfc2);

    uarf_pfc_start(&pfc);
    uarf_pfc_start(&pfc2);

    func1_loop();

    uarf_pfc_stop(&pfc2);
    uarf_pfc_stop(&pfc);

    uint64_t count = uarf_pfc_read(&pfc);
    uint64_t count2 = uarf_pfc_read(&pfc2);

    printf("count1: %lu\n", count);
    printf("count2: %lu\n", count2);

    UARF_TEST_ASSERT(count2 == FUNC1_LOOP_INSNS_SYS);
    UARF_TEST_ASSERT(count >= count2);

    UARF_TEST_PASS();
}

uarf_psnip_declare(rdpmc, psnip_rdpmc);
uarf_psnip_declare_define(psnip_ret, "ret\n\t");

uarf_psnip_declare_define(psnip_counter_init, "movq $0, %rdi\n\t");
uarf_psnip_declare_define(psnip_counter_inc, "inc %rdi\n\t");
uarf_psnip_declare_define(psnip_counter_jmp, "int3\n\t"
                                             "int3\n\t"
                                             "cmp $5, %rdi\n\t"
                                             "je .+16\n\t");

uarf_psnip_declare_define(psnip_reg_ind_br, "lea return_here1(%rip), %rsi\n\t"
                                            "jmp *%rsi\n\t"
                                            "return_here1:\n\t");

uarf_psnip_declare_define(psnip_mem_ind_br, "lea return_here2(%rip), %rsi\n\t"
                                            "pushq %rsi\n\t"
                                            "jmp *(%rsp)\n\t"
                                            "return_here2:\n\t"
                                            "popq %rsi\n\t");

UARF_TEST_CASE(raw_asm) {
    UarfPfc pfc1;
    UarfPfc pfc2;
    UarfPfc pfc3;

    (void) pfc3;

    UarfPfcConfig config1 = (UarfPfcConfig) {
#if UARF_IS_AMD()
        .pmu_conf = UARF_AMD_EX_RET_BRN_IND_MISP,
#endif
        .exclude = UARF_PFC_EXCLUDE_KERNEL};

    UarfPfcConfig config2 = (UarfPfcConfig) {
#if UARF_IS_AMD()
        .pmu_conf = UARF_AMD_EX_RET_IND_BRCH_INSTR,
#endif
        .exclude = UARF_PFC_EXCLUDE_KERNEL};

    uarf_pfc_init(&pfc1, config1);
    uarf_pfc_init(&pfc2, config2);
    // uarf_pfc_init(&pfc3, UARF_AMD_EX_RET_INSTR, UARF_PFC_EXCLUDE_KERNEL);

    UarfJitaCtxt jita = uarf_jita_init();
    UarfStub stub = uarf_stub_init();

    // We actually only need the stacks it contains
    UarfSpecData data;
    uarf_cstack_reset(&data.ustack);
    uarf_cstack_reset(&data.istack);
    uarf_cstack_reset(&data.ostack);

    // Push PFC indexes to the input stack
    // Start measure
    uarf_cstack_push(&data.istack, pfc1.rdpmc_index);
    uarf_cstack_push(&data.istack, pfc2.rdpmc_index);
    // uarf_cstack_push(&data.istack, pfc3.index);

    // Stop measure
    uarf_cstack_push(&data.istack, pfc1.rdpmc_index);
    uarf_cstack_push(&data.istack, pfc2.rdpmc_index);
    // uarf_cstack_push(&data.istack, pfc3.index);

    // UARF_TEST_ASSERT(data.ustack.index == 0);
    // UARF_TEST_ASSERT(data.istack.index == 6);
    // UARF_TEST_ASSERT(data.ostack.index == 0);

    uarf_jita_push_psnip(&jita, &psnip_counter_init);

    // Start measure
    uarf_jita_push_psnip(&jita, &psnip_rdpmc);
    uarf_jita_push_psnip(&jita, &psnip_rdpmc);
    // uarf_jita_push_psnip(&jita, &psnip_rdpmc);

    uarf_jita_push_vsnip_fill_nop(&jita, 64);

    // Measured section
    uarf_jita_push_psnip(&jita, &psnip_reg_ind_br);
    uarf_jita_push_psnip(&jita, &psnip_mem_ind_br);

    uarf_jita_push_psnip(&jita, &psnip_counter_inc);
    uarf_jita_push_psnip(&jita, &psnip_counter_jmp);

    uarf_jita_push_vsnip_jmp_near_rel(&jita, -32, false);

    uarf_jita_push_vsnip_fill_nop(&jita, 64);

    // Stop measure
    uarf_jita_push_psnip(&jita, &psnip_rdpmc);
    uarf_jita_push_psnip(&jita, &psnip_rdpmc);
    // uarf_jita_push_psnip(&jita, &psnip_rdpmc);

    uarf_jita_push_psnip(&jita, &psnip_ret);

    uarf_jita_allocate(&jita, &stub, uarf_rand47());
    data.spec_prim_p = stub.addr;

    uarf_run_spec(&data);

    // UARF_TEST_ASSERT(data.ustack.index == 0);
    // UARF_TEST_ASSERT(data.istack.index == 0);
    // UARF_TEST_ASSERT(data.ostack.index == 12);

    // Get counter values
    // uint64_t pfc3_end_lo = uarf_cstack_pop(&data.ostack);
    // uint64_t pfc3_end_hi = uarf_cstack_pop(&data.ostack);
    uint64_t pfc2_end_lo = uarf_cstack_pop(&data.ostack);
    uint64_t pfc2_end_hi = uarf_cstack_pop(&data.ostack);
    uint64_t pfc1_end_lo = uarf_cstack_pop(&data.ostack);
    uint64_t pfc1_end_hi = uarf_cstack_pop(&data.ostack);

    // uint64_t pfc3_start_lo = uarf_cstack_pop(&data.ostack);
    // uint64_t pfc3_start_hi = uarf_cstack_pop(&data.ostack);
    uint64_t pfc2_start_lo = uarf_cstack_pop(&data.ostack);
    uint64_t pfc2_start_hi = uarf_cstack_pop(&data.ostack);
    uint64_t pfc1_start_lo = uarf_cstack_pop(&data.ostack);
    uint64_t pfc1_start_hi = uarf_cstack_pop(&data.ostack);

    UARF_TEST_ASSERT(data.ustack.index == 0);
    UARF_TEST_ASSERT(data.istack.index == 0);
    UARF_TEST_ASSERT(data.ostack.index == 0);

    uint64_t pfc1_count = uarf_pm_transform_raw1(&pfc1, pfc1_end_lo, pfc1_end_hi) -
                          uarf_pm_transform_raw1(&pfc1, pfc1_start_lo, pfc1_start_hi);

    uint64_t pfc2_count = uarf_pm_transform_raw1(&pfc2, pfc2_end_lo, pfc2_end_hi) -
                          uarf_pm_transform_raw1(&pfc2, pfc2_start_lo, pfc2_start_hi);

    // uint64_t pfc3_count = uarf_pm_transform_raw1(&pfc3, pfc3_end_lo,
    // pfc3_end_hi) -
    //                       uarf_pm_transform_raw1(&pfc3, pfc3_start_lo,
    //                       pfc3_start_hi);

    printf("pfc1: %lu\n", pfc1_count);
    printf("pfc2: %lu\n", pfc2_count);
    // printf("pfc3: %lu\n", pfc3_count);

    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {
    // uarf_log_system_base_level = UARF_LOG_LEVEL_TRACE;

    UARF_TEST_RUN_CASE(man);
    UARF_TEST_RUN_CASE(basic);
    UARF_TEST_RUN_CASE(rdpmc);
    UARF_TEST_RUN_CASE(two_pmc);
    // UARF_TEST_RUN_CASE(raw_asm);
    return 0;
}
