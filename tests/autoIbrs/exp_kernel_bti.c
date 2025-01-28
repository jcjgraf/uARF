/**
 * Automatic IBRS BTI on Host
 *
 * That is the impact of Automatic IBRS on BTI? Is is still possible? We only consider
 * Host user and supervisor in this experiment (no guest).
 *
 * We consider the attack vectors:
 *  - U -> U (basecase)
 *  - K -> K
 *  - K -> U
 *  - U -> K
 *
 * We test the mentioned attack vectors, toggle autoIBRS to see its impacts. The results
 * of this experiment are summarized in this table:
 *
 *        | no autoIBRS | autoIBRS |
 * ------ | ----------- | -------- |
 * U -> U | 1.0         | 1.0      |
 * K -> K | 0.85        | 0        |
 * K -> U | 1.0         | 1.0      |
 * U -> K | 0.76        | 0        |
 *
 * The results are inline with the assumption, that autoIBRS disables the speculative
 * execution of indirect branch targets.
 *
 * Interesting is that K -> U interference works, even with autoIBRS. This implies that it
 * really only disables the execution, but still updates the state.
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
    bool auto_ibrs;
};

TEST_CASE_ARG(basic, arg) {
    struct TestCaseData *data = (struct TestCaseData *) arg;
    srand(data->seed);

    LOG_INFO("%s -> %s, autoIBRS %s\n", data->train_in_kernel ? "HS" : "HU",
             data->signal_in_kernel ? "HS" : "HU", data->auto_ibrs ? "yes" : "no");

    // struct FrConfig fr = fr_init(8, 6, (size_t[]){0, 1, 2, 3, 5, 10});
    struct FrConfig fr = fr_init(8, 1, NULL);

    stub_t stub_main = stub_init();
    stub_t stub_gadget = stub_init();
    stub_t stub_dummy = stub_init();

    void (*train_f)(void *func_p, void *data) =
        data->train_in_kernel ? rap_call : rup_call;
    void (*signal_f)(void *func_p, void *data) =
        data->signal_in_kernel ? rap_call : rup_call;

    IBPB();

    if (data->auto_ibrs) {
        AUTO_IBRS_ON();
    }
    else {
        AUTO_IBRS_OFF();
    }

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

            // Ensure the required data is paged, as when executing in supervisor mode we
            // cannot handle pagefaults => error
            *(volatile char *) run_spec;
            *(volatile char *) train_data.spec_prim_p;
            **(volatile char **) train_data.spec_dst_p_p;
            *(volatile char *) train_data.fr_buf_p;
            *(volatile char *) signal_data.spec_prim_p;
            **(volatile char **) signal_data.spec_dst_p_p;
            *(volatile char *) signal_data.fr_buf_p;

            for (size_t t = 0; t < data->num_train_rounds; t++) {
                asm("train:\n\t");
                train_f(run_spec, &train_data);
            }

            fr_flush(&fr);
            clflush_spec_dst(&signal_data);
            // invlpg_spec_dst(&signal_data);
            prefetcht0(&train_data);

            // TODO: disable interrupts and preemption
            asm("signal:\n\t");
            signal_f(run_spec, &signal_data);

            fr_reload_binned(&fr, r);
        }
        jita_deallocate(data->jita_main, &stub_main);
        jita_deallocate(data->jita_gadget, &stub_gadget);
        jita_deallocate(data->jita_dummy, &stub_dummy);
    }

    LOG_INFO("Done\n");

    fr_print(&fr);

    fr_deinit(&fr);

    TEST_PASS();
}

static struct TestCaseData data[10];

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

    size_t data_i = 0;

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = false,
        .auto_ibrs = false,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = false,
        .auto_ibrs = false,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = false,
        .auto_ibrs = false,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = false,
        .auto_ibrs = true,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = true,
        .auto_ibrs = false,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = false,
        .signal_in_kernel = true,
        .auto_ibrs = true,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = true,
        .auto_ibrs = false,
    };

    data[data_i++] = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .train_in_kernel = true,
        .signal_in_kernel = true,
        .auto_ibrs = true,
    };

    for (size_t i = 0; i < data_i; i++) {
        RUN_TEST_CASE_ARG(basic, data + i);
    }

    pi_deinit();
    rap_deinit();

    return 0;
}
