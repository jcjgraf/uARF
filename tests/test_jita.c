/*
 * Name: test_jita
 * Desc: Test jita, using snips and stubs
 */

#define TEST_NAME test_jita

#include "compiler.h"
#include "jita.h"
#include "lib.h"
#include "log.h"
#include "psnip.h"
#include "stub.h"
#include "test.h"
#include "utils.h"
#include "vsnip.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_TEST
#endif

#define ADDR 0x0000701100001000ul

// Declare all used snippets
psnip_declare(jita_inc, psnip_inc);
psnip_declare(jita_dec, psnip_dec);
psnip_declare(jita_jump, psnip_jump);
psnip_declare(jita_ret_val, psnip_ret_val);

stub_t stub1;

jita_ctxt_t jita_inc3;
jita_ctxt_t jita_get_inc3;

// Use global jita that has been allocated to a global stub
TEST_CASE(globally_allocated_jita) {

    int (*a)(int) = (int (*)(int)) stub1.ptr;
    int var = a(5);

    ASSERT(var == 8);

    return 0;
}

// Use a global jita and allocate to local stub
TEST_CASE(locally_allocated_jita) {

    stub_t local_stub;
    jita_allocate(&jita_get_inc3, &local_stub, rand47());

    int (*a)(int) = (int (*)(int)) local_stub.ptr;
    int var = a(5);

    ASSERT(var == 8);

    jita_deallocate(&jita_get_inc3, &local_stub);

    return 0;
}

// Local jita
TEST_CASE(local_jita) {

    jita_ctxt_t ctxt = jita_get_ctxt();
    stub_t local_stub;

    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_ret_val);

    jita_allocate(&ctxt, &local_stub, rand47());

    int (*a)(int) = (int (*)(int)) local_stub.ptr;
    int var = a(5);

    ASSERT(var == 8);

    jita_deallocate(&ctxt, &local_stub);

    return 0;
}

// Clone global jita and extend locally.
TEST_CASE(global_jita_clone_local) {

    jita_ctxt_t ctxt = jita_get_ctxt();
    stub_t local_stub;

    jita_clone(&jita_inc3, &ctxt);

    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_jump);
    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_jump);
    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_jump);
    jita_push_psnip(&ctxt, &psnip_ret_val);

    jita_allocate(&ctxt, &local_stub, rand47());

    int (*a)(int) = (int (*)(int)) local_stub.ptr;
    int var = a(5);

    ASSERT(var == 11);

    jita_deallocate(&ctxt, &local_stub);

    return 0;
}

// Test vsnip_dump_stub
TEST_CASE(vsnip_dump_stub) {

    jita_ctxt_t ctxt = jita_get_ctxt();
    stub_t local_stub;
    stub_t dump_stub;

    jita_clone(&jita_get_inc3, &ctxt);
    jita_push_vsnip_dump_stub(&ctxt, &dump_stub);
    jita_allocate(&ctxt, &local_stub, ALIGN_UP(rand47(), PAGE_SIZE));
    ASSERT(local_stub.base_addr == dump_stub.base_addr);
    ASSERT(local_stub.size == dump_stub.size);
    ASSERT(local_stub.addr == dump_stub.addr);
    ASSERT(local_stub.end_addr == dump_stub.end_addr);
    jita_deallocate(&jita_get_inc3, &local_stub);

    jita_push_psnip(&ctxt, &psnip_inc);
    jita_allocate(&ctxt, &local_stub, rand47());
    ASSERT(local_stub.base_addr == dump_stub.base_addr);
    ASSERT(local_stub.size == dump_stub.size);
    ASSERT(local_stub.addr == dump_stub.addr);
    ASSERT(local_stub.end_addr == dump_stub.end_addr + psnip_size(&psnip_inc));
    jita_deallocate(&jita_get_inc3, &local_stub);

    return 0;
}

// Test vsnip_aligned and vsnip_assert_align
// Not checking for false negatives of vsnip_aligned
TEST_CASE_ARG(vsnip_align, arg) {
    uint64_t alignment = _ul(arg);

    jita_ctxt_t ctxt = jita_get_ctxt();
    stub_t local_stub;
    stub_t dump_stub;

    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_vsnip_align(&ctxt, alignment);
    jita_push_vsnip_assert_align(&ctxt, alignment);
    jita_push_vsnip_dump_stub(&ctxt, &dump_stub);
    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_ret_val);

    uint64_t addr = rand47();

    jita_allocate(&ctxt, &local_stub, addr);

    ASSERT(!memcmp(local_stub.ptr, psnip_inc.ptr, psnip_size(&psnip_inc)));

    uint64_t cur = local_stub.addr + psnip_size(&psnip_inc);
    while (*(uint8_t *) cur == 0x90) {
        cur++;
    }
    ASSERT(ALIGN_UP(cur, alignment) == cur);

    ASSERT(cur == dump_stub.end_addr);

    ASSERT(!memcmp(_ptr(cur), psnip_inc.ptr, psnip_size(&psnip_inc)));
    cur += psnip_size(&psnip_inc);
    ASSERT(!memcmp(_ptr(cur), psnip_ret_val.ptr, psnip_size(&psnip_ret_val)));

    int (*a)(int) = (int (*)(int)) local_stub.ptr;
    int var = a(5);

    ASSERT(var == 7);

    jita_deallocate(&ctxt, &local_stub);

    return 0;
}

// Aligning an already aligned address does not add more padding
TEST_CASE(vsnip_align_aligned) {
    jita_ctxt_t ctxt = jita_get_ctxt();
    stub_t local_stub;
    stub_t dump_stub1;
    stub_t dump_stub2;

    jita_clone(&jita_get_inc3, &ctxt);

    jita_push_vsnip_align(&ctxt, 8);
    jita_push_vsnip_dump_stub(&ctxt, &dump_stub1);
    jita_push_vsnip_align(&ctxt, 8);
    jita_push_vsnip_dump_stub(&ctxt, &dump_stub2);

    jita_push_psnip(&ctxt, &psnip_inc);

    jita_allocate(&ctxt, &local_stub, rand47());
    ASSERT(dump_stub1.base_addr == dump_stub2.base_addr);
    ASSERT(dump_stub1.addr == dump_stub2.addr);
    ASSERT(dump_stub1.end_addr == dump_stub2.end_addr);
    ASSERT(dump_stub1.size == dump_stub2.size);

    jita_deallocate(&ctxt, &local_stub);

    return 0;
}

// Test vsnip_jmp_near_abs
TEST_CASE(vsnip_jmp_near_abs) {
    jita_ctxt_t jmp_ctxt = jita_get_ctxt();
    jita_ctxt_t target_ctxt = jita_get_ctxt();

    stub_t jmp_stub;
    stub_t target_stub;

    uint64_t jmp_src_addr = rand47();
    uint64_t jmp_target_addr = jmp_src_addr ^ (rand() & 0xFFFFFF);
    ASSERT(ALIGN_DOWN(jmp_target_addr, PAGE_SIZE) >= ALIGN_DOWN(jmp_src_addr, PAGE_SIZE));

    jita_push_psnip(&target_ctxt, &psnip_inc);
    jita_push_psnip(&target_ctxt, &psnip_ret_val);
    jita_push_vsnip_jmp_near_abs(&jmp_ctxt, jmp_target_addr);
    jita_push_psnip(&jmp_ctxt, &psnip_dec); // Must not change value

    jita_allocate(&jmp_ctxt, &jmp_stub, jmp_src_addr);
    jita_allocate(&target_ctxt, &target_stub, jmp_target_addr);

    int (*a)(int) = (int (*)(int)) jmp_stub.ptr;
    int var = a(5);

    ASSERT(var == 6);

    jita_deallocate(&jmp_ctxt, &jmp_stub);
    jita_deallocate(&target_ctxt, &target_stub);

    return 0;
}

// Test vsnip_jmp_near_rel having the offset be inclusive
TEST_CASE(vsnip_jmp_near_rel_inclusive) {
    jita_ctxt_t jmp_ctxt = jita_get_ctxt();
    jita_ctxt_t target_ctxt = jita_get_ctxt();

    stub_t jmp_stub;
    stub_t target_stub;
    stub_t jmp_dump;

    uint64_t jmp_src_addr = rand47();
    uint32_t offset = rand() & 0xFFFFFF;
    uint64_t jmp_target_addr = jmp_src_addr + offset;
    ASSERT(ALIGN_DOWN(jmp_target_addr, PAGE_SIZE) >= ALIGN_DOWN(jmp_src_addr, PAGE_SIZE));

    jita_push_psnip(&target_ctxt, &psnip_inc);
    jita_push_psnip(&target_ctxt, &psnip_ret_val);
    jita_push_vsnip_dump_stub(&jmp_ctxt, &jmp_dump);
    jita_push_vsnip_jmp_near_rel(&jmp_ctxt, offset, true);
    jita_push_psnip(&jmp_ctxt, &psnip_dec); // Must not change value

    jita_allocate(&jmp_ctxt, &jmp_stub, jmp_src_addr);
    jita_allocate(&target_ctxt, &target_stub, jmp_target_addr);

    ASSERT(jmp_dump.end_addr + offset == target_stub.addr);

    int (*a)(int) = (int (*)(int)) jmp_stub.ptr;
    int var = a(5);

    ASSERT(var == 6);

    jita_deallocate(&jmp_ctxt, &jmp_stub);
    jita_deallocate(&target_ctxt, &target_stub);

    return 0;
}

// Test vsnip_jmp_near_rel having the offset be exclusive
TEST_CASE(vsnip_jmp_near_rel_exclusive) {
    jita_ctxt_t jmp_ctxt = jita_get_ctxt();
    jita_ctxt_t target_ctxt = jita_get_ctxt();

    stub_t jmp_stub;
    stub_t target_stub;
    stub_t jmp_dump;

    uint64_t jmp_src_addr = rand47();
    uint32_t offset = rand() & 0xFFFFFF;
    uint64_t jmp_target_addr = jmp_src_addr + offset + 5;
    ASSERT(ALIGN_DOWN(jmp_target_addr, PAGE_SIZE) >= ALIGN_DOWN(jmp_src_addr, PAGE_SIZE));

    LOG_DEBUG("offset %u\n", offset);

    jita_push_psnip(&target_ctxt, &psnip_inc);
    jita_push_psnip(&target_ctxt, &psnip_ret_val);
    jita_push_vsnip_jmp_near_rel(&jmp_ctxt, offset, false);
    jita_push_vsnip_dump_stub(&jmp_ctxt, &jmp_dump);
    jita_push_psnip(&jmp_ctxt, &psnip_dec); // Must not change value

    jita_allocate(&jmp_ctxt, &jmp_stub, jmp_src_addr);
    jita_allocate(&target_ctxt, &target_stub, jmp_target_addr);

    ASSERT(jmp_dump.end_addr + offset == target_stub.addr);

    int (*a)(int) = (int (*)(int)) jmp_stub.ptr;
    int var = a(5);

    ASSERT(var == 6);

    jita_deallocate(&jmp_ctxt, &jmp_stub);
    jita_deallocate(&target_ctxt, &target_stub);

    return 0;
}

// Test vsnip_fill
static unsigned long __text vsnip_fill() {
    jita_ctxt_t ctxt = jita_get_ctxt();
    stub_t stub;
    stub_t dump_before;
    stub_t dump_after;

    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_vsnip_dump_stub(&ctxt, &dump_before);
    jita_push_vsnip_fill_nop(&ctxt, 42);
    jita_push_vsnip_dump_stub(&ctxt, &dump_after);
    jita_push_psnip(&ctxt, &psnip_inc);
    jita_push_psnip(&ctxt, &psnip_ret_val);

    uint64_t addr = rand47();

    jita_allocate(&ctxt, &stub, addr);

    // Before is no nop
    ASSERT(*(uint8_t *) (dump_before.end_addr - 1) != 0x90);

    // In-between is all nops
    for (size_t i = 0; i < 42; i++) {
        ASSERT(*(uint8_t *) (dump_before.end_addr + i) == 0x90);
    }

    // After is no nop
    ASSERT(*(uint8_t *) (dump_after.end_addr) != 0x90);
    ASSERT(dump_after.end_addr = dump_before.end_addr + 42);

    int (*a)(int) = (int (*)(int)) stub.ptr;
    int var = a(5);

    ASSERT(var == 7);

    return 0;
}

TEST_SUITE(TEST_NAME) {

    jita_push_psnip(&jita_inc3, &psnip_inc);
    jita_push_psnip(&jita_inc3, &psnip_inc);
    jita_push_psnip(&jita_inc3, &psnip_inc);

    jita_clone(&jita_inc3, &jita_get_inc3);
    jita_push_psnip(&jita_get_inc3, &psnip_ret_val);

    jita_allocate(&jita_get_inc3, &stub1, rand47());

    RUN_TEST_CASE(globally_allocated_jita);
    RUN_TEST_CASE(locally_allocated_jita);
    RUN_TEST_CASE(local_jita);
    RUN_TEST_CASE(global_jita_clone_local);
    RUN_TEST_CASE(vsnip_dump_stub);
    RUN_TEST_CASE_ARG(vsnip_align, _ptr(1));
    RUN_TEST_CASE_ARG(vsnip_align, _ptr(2));
    RUN_TEST_CASE_ARG(vsnip_align, _ptr(4));
    RUN_TEST_CASE_ARG(vsnip_align, _ptr(8));
    RUN_TEST_CASE_ARG(vsnip_align, _ptr(16));
    RUN_TEST_CASE(vsnip_align_aligned);
    RUN_TEST_CASE(vsnip_jmp_near_abs);
    RUN_TEST_CASE(vsnip_jmp_near_rel_inclusive);
    RUN_TEST_CASE(vsnip_jmp_near_rel_exclusive);
    RUN_TEST_CASE(vsnip_fill);

    return 0;
}
