#pragma once

// Offsets into struct Data
// WARNING: Do not change without changing struct SpecData
#define DATA__spec_prim_p  0
#define DATA__spec_dst_p_p 8
#define DATA__fr_buf_p     16
#define DATA__secret       24
#define DATA__hist         32
#define DATA__hist_size    8
// #define DATA__hist_num   200

#ifndef __ASSEMBLY__
#include "compiler.h"
#include "kmod/rap.h"
#include "lib.h"
#include <stdint.h>

struct history {
    uint64_t hist[2];
};

// Holds all the values that a indirect branch stub requires
// WARNING: Adjust the DATA__ macros when changing this!
struct SpecData {
    uint64_t spec_prim_p;  // 0
    uint64_t spec_dst_p_p; // 8
    uint64_t fr_buf_p;     // 16
    uint64_t secret;       // 24
    struct history hist;   // 32 onwards
} __aligned(8) __packed;

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

#endif // __ASSEMBLY__
