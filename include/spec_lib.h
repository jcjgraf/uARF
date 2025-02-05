#pragma once

// Offsets into struct Data
// WARNING: Do not change without changing struct SpecData
#define DATA__spec_prim_p    0
#define DATA__spec_dst_p_p   8
#define DATA__fr_buf_p       16
#define DATA__secret         24
#define DATA__ustack_index   32
#define DATA__ustack_stack   40
#define DATA__ustack_scratch 168
#define DATA__istack_index   176
#define DATA__istack_stack   184
#define DATA__istack_scratch 312
#define DATA__ostack_index   320
#define DATA__ostack_stack   328
#define DATA__ostack_scratch 456
#define DATA__hist           464
#define DATA__hist_size      8
// #define DATA__hist_num   200

#define USTACK_INDEX_OFFSET   DATA__ustack_index
#define USTACK_STACK_OFFSET   DATA__ustack_stack
#define USTACK_SCRATCH_OFFSET DATA__ustack_scratch

#define ISTACK_INDEX_OFFSET   DATA__istack_index
#define ISTACK_STACK_OFFSET   DATA__istack_stack
#define ISTACK_SCRATCH_OFFSET DATA__istack_scratch

#define OSTACK_INDEX_OFFSET   DATA__ostack_index
#define OSTACK_STACK_OFFSET   DATA__ostack_stack
#define OSTACK_SCRATCH_OFFSET DATA__ostack_scratch

#ifndef __ASSEMBLY__
#include "compiler.h"
#include "kmod/rap.h"
#include "lib.h"
#include <stdint.h>

struct history {
    uint64_t hist[2];
};

#define CSTACK_LEN 16
struct CStack {
    uint64_t index;
    uint64_t buffer[16];
    uint64_t scratch;
} __packed;

// Holds all the values that a indirect branch stub requires
// WARNING: Adjust the DATA__ macros when changing this!
struct SpecData {
    uint64_t spec_prim_p;  // 0
    uint64_t spec_dst_p_p; // 8
    uint64_t fr_buf_p;     // 16
    uint64_t secret;       // 24
    struct CStack ustack;  // 32
    struct CStack istack;  // 176
    struct CStack ostack;  // 320
    struct history hist;   // 464 onwards
} __aligned(8) __packed;

__always_inline void cstack_push(struct CStack *stk, uint64_t value) {
    stk->buffer[stk->index++] = value;
}

__always_inline uint64_t cstack_pop(struct CStack *stk) {
    return stk->buffer[--stk->index];
}

__always_inline void cstack_reset(struct CStack *stk) {
    *stk = (struct CStack) {.index = 0, .buffer = {0}, .scratch = 0};
}

// Get an randomly initialized history
struct history get_randomized_history(void);

static inline void clflush_spec_dst(const struct SpecData *data) {
    clflush(_ptr(*(uint64_t *) data->spec_dst_p_p));
}

static inline void invlpg_spec_dst(const struct SpecData *data) {
    invlpg(_ptr(*(uint64_t *) data->spec_dst_p_p));
}

static inline void prefetcht0_spec_dst(const struct SpecData *data) {
    prefetcht0(_ptr(*(uint64_t *) data->spec_dst_p_p));
}

// Run the speculation primitive described by SpecData.
void run_spec(void *arg);

#endif // __ASSEMBLY__
